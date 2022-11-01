// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ScanningServerConnectionThread.h"

#include "ScanRequest.capnp.h"
#include "ThreatDetectedMessageUtils.h"

#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include "common/FDUtils.h"
#include "common/SaferStrerror.h"
#include "common/ShuttingDownException.h"
#include "common/StringUtils.h"
#include "datatypes/SystemCallWrapper.h"

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

bool unixsocket::ScanningServerConnectionThread::sendResponse(datatypes::AutoFd& socket_fd, const scan_messages::ScanResponse& response)
{
    std::string serialised_result = response.serialise();
    try
    {
        if (!writeLengthAndBuffer(socket_fd, serialised_result))
        {
            LOGWARN("Failed to write result to unix socket");
            return false;
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGWARN("Exiting Scanning Connection Thread: " << e.what());
        return false;
    }
    return true;
}

void unixsocket::ScanningServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_socketFd));
    LOGDEBUG("Scanning Server thread got connection " << socket_fd.fd());
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);
    auto sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();

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

        int activity = sysCallWrapper->pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);

        if (activity < 0)
        {
            LOGERROR("Closing Scanning connection thread because pselect failed: " << common::safer_strerror(errno));
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
            scan_messages::ScanResponse result;
            ssize_t bytes_read;
            std::string errMsg;
            if (!readCapnProtoMsg(sysCallWrapper, length, buffer_size, proto_buffer, socket_fd, bytes_read, loggedLengthOfZero, errMsg))
            {
                result.setErrorMsg(errMsg);
                sendResponse(socket_fd, result);
                LOGERROR(errMsg);
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
                errMsg = "Aborting Scanning connection thread: failed to read fd";
                result.setErrorMsg(errMsg);
                sendResponse(socket_fd, result);
                LOGERROR(errMsg);
                break;
            }
            LOGDEBUG("Managed to get file descriptor: " << file_fd.get());

            // Keep the connection open if we can read the message but get a file that we can't scan
            if (!isReceivedFdFile(sysCallWrapper, file_fd, errMsg) || !isReceivedFileOpen(sysCallWrapper, file_fd, errMsg))
            {
                result.setErrorMsg(errMsg);
                sendResponse(socket_fd, result);
                LOGERROR(errMsg);
                continue;
            }

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
                errMsg = "Aborting scan, scanner is shutting down";
                result.setErrorMsg(errMsg);
                sendResponse(socket_fd, result);
                LOGERROR(errMsg);
                break;
            }

            file_fd.reset();

            if (!sendResponse(socket_fd, result))
            {
                break;
            }
        }
    }
}