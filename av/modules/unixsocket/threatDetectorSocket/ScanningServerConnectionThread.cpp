// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ScanningServerConnectionThread.h"

#include "ScanRequest.capnp.h"

#include <scan_messages/ScanRequest.h>

#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include "common/StringUtils.h"
#include "common/ShuttingDownException.h"
#include <common/FDUtils.h>

#include <capnp/serialize.h>

#include <cassert>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

unixsocket::ScanningServerConnectionThread::ScanningServerConnectionThread(
        datatypes::AutoFd& fd,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory,
        int maxIterations)
    : m_socketFd(std::move(fd))
    , m_scannerFactory(std::move(scannerFactory))
    , m_maxIterations(maxIterations)
{
    if (m_socketFd < 0)
    {
        throw std::runtime_error("Attempting to construct ScanningServerConnectionThread with invalid socket fd");
    }

    if (m_scannerFactory.get() == nullptr)
    {
        throw std::runtime_error("Attempting to construct ScanningServerConnectionThread with null scanner factory");
    }
}

//
//static void throwOnError(int ret, const std::string& message)
//{
//    if (ret == 0)
//    {
//        return;
//    }
//    perror(message.c_str());
//    throw std::runtime_error(message);
//}

/**
 * Parse a request.
 *
 * Placed in a function to ensure the MessageReader and view are deleted before the buffer might become invalid.
 *
 * @param proto_buffer
 * @param bytes_read
 * @return
 */
static std::shared_ptr<scan_messages::ScanRequest> parseRequest(kj::Array<capnp::word>& proto_buffer, ssize_t& bytes_read)
{
    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::FileScanRequest::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::FileScanRequest>();

    std::shared_ptr<scan_messages::ScanRequest> request = std::make_shared<scan_messages::ScanRequest>(requestReader);

    return request;
}

void unixsocket::ScanningServerConnectionThread::run()
{
    setIsRunning(true);
    announceThreadStarted();

    // LINUXDAR-4543: Block signals in this thread, and all threads started from this thread
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGUSR1);
    sigaddset(&signals, SIGTERM);
    sigaddset(&signals, SIGHUP);
    sigaddset(&signals, SIGQUIT);
    int s = pthread_sigmask(SIG_BLOCK, &signals, nullptr);
    std::ignore = s; assert(s == 0);

    // Start server:
    LOGDEBUG("Starting Scanning Server thread");

    try
    {
        inner_run();
    }
    catch (const kj::Exception& ex)
    {
        if (ex.getType() == kj::Exception::Type::UNIMPLEMENTED)
        {
            // Fatal since this means we have a coding error that calls something unimplemented in kj.
            LOGFATAL("Terminated ScanningServerConnectionThread with serialisation unimplemented exception: "
                     << ex.getDescription().cStr());
        }
        else
        {
            LOGERROR(
                "Terminated ScanningServerConnectionThread with serialisation exception: "
                << ex.getDescription().cStr());
        }
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Terminated ScanningServerConnectionThread with exception: " << ex.what());
    }
    catch (...)
    {
        // Fatal since this means we have thrown something that isn't a subclass of std::exception
        LOGFATAL("Terminated ScanningServerConnectionThread with unknown exception");
    }

    LOGDEBUG("Stopping Scanning Server thread");

    setIsRunning(false);
}

void unixsocket::ScanningServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_socketFd));
    LOGDEBUG("Scanning Server thread got connection " << socket_fd.fd());
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    int exitFD = m_notifyPipe.readFd();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = FDUtils::addFD(&readFDs, exitFD, max);
    max = FDUtils::addFD(&readFDs, socket_fd, max);
    threat_scanner::IThreatScannerPtr scanner;
    bool loggedLengthOfZero = false;

    while (true)
    {
        // if m_maxIterations < 0, iterate forever, otherwise count down and break.
        if (m_maxIterations >= 0)
        {
            if (m_maxIterations-- == 0)
            {
                break;
            }
        }

        fd_set tempRead = readFDs;

        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);

        if (activity < 0)
        {
            LOGERROR("Closing Scanning connection thread because pselect failed: " << errno);
            break;
        }
        // We don't set a timeout so something should have happened
        assert(activity != 0);

        if (FDUtils::fd_isset(exitFD, &tempRead))
        {
            LOGSUPPORT("Closing Scanning connection thread");
            break;
        }
        else // if(FDUtils::fd_isset(socket_fd, &tempRead))
        {
            // If shouldn't be required - we have no timeout, and only 2 FDs in the pselect.
            // exitFD will cause break
            // therefore "else" must be fd_isset(socket_fd, &tempRead)
            assert(FDUtils::fd_isset(socket_fd, &tempRead));
            // read length
            int32_t length = unixsocket::readLength(socket_fd);
            if (length == -2)
            {
                LOGDEBUG("Scanning connection thread closed: EOF");
                break;
            }
            else if (length < 0)
            {
                LOGDEBUG("Aborting Scanning connection thread: failed to read length");
                break;
            }
            else if (length == 0)
            {
                if (not loggedLengthOfZero)
                {
                    LOGDEBUG("Ignoring length of zero / No new messages");
                    loggedLengthOfZero = true;
                }
                continue;
            }

            // read capn proto
            if (static_cast<uint32_t>(length) > (buffer_size * sizeof(capnp::word)))
            {
                buffer_size = 1 + length / sizeof(capnp::word);
                proto_buffer = kj::heapArray<capnp::word>(buffer_size);
                loggedLengthOfZero = false;
            }

            ssize_t bytes_read = ::read(socket_fd, proto_buffer.begin(), length);
            if (bytes_read < 0)
            {
                LOGERROR("Aborting Scanning connection thread: " << errno);
                break;
            }
            else if (bytes_read != length)
            {
                LOGERROR("Aborting Scanning connection thread: failed to read entire message");
                break;
            }

            LOGDEBUG("Read capn of " << bytes_read);

            std::shared_ptr<scan_messages::ScanRequest> requestReader = parseRequest(proto_buffer, bytes_read);

            std::string escapedPath(requestReader->getPath());
            common::escapeControlCharacters(escapedPath);

            LOGDEBUG("Scan requested of " << escapedPath);

            // read fd
            datatypes::AutoFd file_fd(unixsocket::recv_fd(socket_fd));
            if (file_fd.get() < 0)
            {
                LOGERROR("Aborting Scanning connection thread: failed to read fd");
                break;
            }
            LOGDEBUG("Managed to get file descriptor: " << file_fd.get());

            // is it a file?
            struct ::stat st {};
            int ret = ::fstat(file_fd.get(), &st);
            if (ret == -1)
            {
                LOGERROR("Aborting Scanning connection thread: failed to get file status");
                break;
            }
            if (!S_ISREG(st.st_mode))
            {
                LOGERROR("Aborting Scanning connection thread: fd is not a regular file");
                break;
            }

            // is it open for read?
            int status = ::fcntl(file_fd.get(), F_GETFL);
            if (status == -1)
            {
                LOGERROR("Aborting Scanning connection thread: failed to get file status flags");
                break;
            }
            unsigned int mode = status & O_ACCMODE;
            if (!(mode == O_RDONLY || mode == O_RDWR ) || status & O_PATH )
            {
                LOGERROR("Aborting Scanning connection thread: fd is not open for read");
                break;
            }

            scan_messages::ScanResponse result;
            try
            {
                if (!scanner)
                {
                    scanner = m_scannerFactory->createScanner(
                        requestReader->scanInsideArchives(), requestReader->scanInsideImages());
                    if (!scanner)
                    {
                        throw std::runtime_error("Failed to create scanner");
                    }
                }

                // The User ID could be spoofed by an untrusted client. Until this is made secure, hardcode it to "n/a"
                result = scanner->scan(file_fd, requestReader->getPath(), requestReader->getScanType(), "n/a");
            }
            catch (ShuttingDownException&)
            {
                LOGINFO("Aborting scan, scanner is shutting down");
                break;
            }

            file_fd.reset();

            std::string serialised_result = result.serialise();

            try
            {
                if (!writeLengthAndBuffer(socket_fd, serialised_result))
                {
                    LOGWARN("Failed to write result to unix socket");
                    break;
                }
            }
            catch (unixsocket::environmentInterruption& e)
            {
                LOGWARN("Exiting Scanning Connection Thread: " << e.what());
                break;
            }
        }
    }
}