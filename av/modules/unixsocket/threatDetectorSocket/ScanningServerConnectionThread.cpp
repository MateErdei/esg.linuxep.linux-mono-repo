// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ScanningServerConnectionThread.h"

#include "ScanRequest.capnp.h"
#include "ThreatDetectedMessageUtils.h"

#include "common/ApplicationPaths.h"
#include "common/FailedToInitializeSusiException.h"
#include "common/SaferStrerror.h"
#include "common/ShuttingDownException.h"
#include "common/StringUtils.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/UnixSocketException.h"

#include "Common/FileSystem/IFileSystem.h"

#include <capnp/serialize.h>

#include <cassert>
#include <csignal>
#include <iostream>
#include <utility>

using namespace scan_messages;

unixsocket::ScanningServerConnectionThread::ScanningServerConnectionThread(
        datatypes::AutoFd& fd,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory,
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  sysCalls,
        int maxIterations)
    : BaseServerConnectionThread("ScanningServerConnectionThread")
    , socketFd_(std::move(fd))
    , scannerFactory_(std::move(scannerFactory))
    , sysCalls_(sysCalls)
    , maxIterations_(maxIterations)
{
    if (socketFd_ < 0)
    {
        throw unixsocket::UnixSocketException(LOCATION, "Attempting to construct " + m_threadName + " with invalid socket fd");
    }

    if (scannerFactory_.get() == nullptr)
    {
        throw unixsocket::UnixSocketException(LOCATION, "Attempting to construct " + m_threadName + " with null scanner factory");
    }
}

/**
 * Parse a request.
 *
 * Placed in a function to ensure the MessageReader and view are deleted before the buffer might become invalid.
 *
 * @param proto_buffer
 * @param bytes_read
 * @return
 */
static std::shared_ptr<ScanRequest> parseRequest(kj::Array<capnp::word>& proto_buffer, ssize_t& bytes_read)
{
    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::FileScanRequest::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::FileScanRequest>();

    std::shared_ptr<ScanRequest> request = std::make_shared<ScanRequest>(requestReader);

    return request;
}

void unixsocket::ScanningServerConnectionThread::run()
{
    setIsRunning(true);

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
    LOGDEBUG("Starting " << m_threadName);

    announceThreadStarted();

    try
    {
        inner_run();
    }
    catch (const kj::Exception& ex)
    {
        if (ex.getType() == kj::Exception::Type::UNIMPLEMENTED)
        {
            // Fatal since this means we have a coding error that calls something unimplemented in kj.
            LOGFATAL("Terminated " << m_threadName << " with serialisation unimplemented exception: "
                     << ex.getDescription().cStr());
        }
        else
        {
            LOGERROR(
                "Terminated " << m_threadName << " with serialisation exception: "
                << ex.getDescription().cStr());
        }
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Terminated " << m_threadName << " with exception: " << ex.what());
    }
    catch (...)
    {
        // Fatal since this means we have thrown something that isn't a subclass of std::exception
        LOGFATAL("Terminated " << m_threadName << " with unknown exception");
    }

    LOGDEBUG("Stopping " << m_threadName);

    setIsRunning(false);
}

bool unixsocket::ScanningServerConnectionThread::sendResponse(datatypes::AutoFd& socket_fd, const ScanResponse& response)
{
    std::string serialised_result = response.serialise();
    try
    {
        if (!writeLengthAndBuffer(socket_fd, serialised_result))
        {
            LOGWARN(m_threadName << " failed to write result to unix socket");
            return false;
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGWARN("Exiting " << m_threadName << ": " << e.what());
        return false;
    }
    return true;
}

bool unixsocket::ScanningServerConnectionThread::attemptScan(
    std::shared_ptr<scan_messages::ScanRequest> scanRequest,
    std::string& errMsg,
    scan_messages::ScanResponse& result,
    datatypes::AutoFd& fd)
{
    auto fs = Common::FileSystem::fileSystem();

    try
    {
        if (!scanner_ || scannerFactory_->detectPUAsEnabled() != scanRequest->detectPUAs())
        {
            scanner_ = scannerFactory_->createScanner(
                scanRequest->scanInsideArchives(),
                scanRequest->scanInsideImages(),
                scanRequest->detectPUAs());
            if (!scanner_)
            {
                throw unixsocket::UnixSocketException(LOCATION, m_threadName + " failed to create scanner");
            }

            fs->removeFile(Plugin::getThreatDetectorUnhealthyFlagPath(), true);
            LOGDEBUG(m_threadName << " has created a new scanner");
        }

        // The User ID could be spoofed by an untrusted client. Until this is made secure, hardcode it to "n/a"
#ifdef USERNAME_UID_USED
# error "Passing UID from untrusted client not supported"
#else
        scanRequest->setUserID("n/a");
#endif
        result = scanner_->scan(fd, *scanRequest);
    }
    catch (FailedToInitializeSusiException& ex)
    {
        errMsg = m_threadName + " aborting scan, failed to initialise SUSI: " + ex.what();
        fs->writeFile(Plugin::getThreatDetectorUnhealthyFlagPath(), "");
        return false;
    }
    catch (ShuttingDownException&)
    {
        errMsg =  m_threadName + " aborting scan, scanner is shutting down";
        return false;
    }
    return true;
}

void unixsocket::ScanningServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(socketFd_));
    LOGDEBUG(m_threadName << " got connection " << socket_fd.fd());
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    struct pollfd fds[] {
        { .fd = socket_fd.get(), .events = POLLIN, .revents = 0 }, // socket FD
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
    };

    bool loggedLengthOfZero = false;

    while (true)
    {
        // if m_maxIterations < 0, iterate forever, otherwise count down and break.
        if (maxIterations_ >= 0)
        {
            if (maxIterations_-- == 0)
            {
                break;
            }
        }

        auto ret = sysCalls_->ppoll(fds, std::size(fds), nullptr, nullptr);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            LOGFATAL(m_threadName << " error from ppoll: " << common::safer_strerror(errno));
            break;
        }
        assert(ret > 0);

        if ((fds[1].revents & POLLERR) != 0)
        {
            LOGERROR("Closing " << m_threadName << ", error from notify pipe");
            break;
        }

        if ((fds[0].revents & POLLERR) != 0)
        {
            LOGDEBUG("Closing " << m_threadName << ", error from socket");
            break;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            LOGSUPPORT("Closing " << m_threadName);
            break;
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
            // read length
            auto length = unixsocket::readLength(socket_fd);
            if (length == -2)
            {
                LOGDEBUG(m_threadName << " closed: EOF");
                break;
            }
            else if (length < 0)
            {
                LOGDEBUG("Aborting " << m_threadName << ": failed to read length");
                break;
            }
            else if (length == 0)
            {
                if (not loggedLengthOfZero)
                {
                    LOGDEBUG(m_threadName << " ignoring length of zero / No new messages");
                    loggedLengthOfZero = true;
                }
                continue;
            }

            // read capn proto
            ScanResponse result;
            ssize_t bytes_read;
            std::string errMsg;
            if (!readCapnProtoMsg(
                    sysCalls_,
                    length,
                    buffer_size,
                    proto_buffer,
                    socket_fd,
                    bytes_read,
                    loggedLengthOfZero,
                    errMsg,
                    readTimeout_))
            {
                result.setErrorMsg(errMsg);
                sendResponse(socket_fd, result);
                LOGERROR(m_threadName << ": " << errMsg);
                break;
            }
            LOGDEBUG(m_threadName << " read capn of " << bytes_read);
            std::shared_ptr<ScanRequest> requestReader = parseRequest(proto_buffer, bytes_read);

            std::string escapedPath(requestReader->getPath());
            common::escapeControlCharacters(escapedPath);

            LOGDEBUG(m_threadName << " scan requested of " << escapedPath);

            // read fd
            datatypes::AutoFd file_fd(unixsocket::recv_fd(socket_fd));
            if (file_fd.get() < 0)
            {
                errMsg = "Aborting " + m_threadName + ": failed to read fd";
                result.setErrorMsg(errMsg);
                sendResponse(socket_fd, result);
                LOGERROR(errMsg);
                break;
            }
            LOGDEBUG(m_threadName << " managed to get file descriptor: " << file_fd.get());

            // Keep the connection open if we can read the message but get a file that we can't scan
            if (!isReceivedFdFile(sysCalls_, file_fd, errMsg) || !isReceivedFileOpen(sysCalls_, file_fd, errMsg))
            {
                result.setErrorMsg(errMsg);
                sendResponse(socket_fd, result);
                LOGERROR(errMsg);
                continue;
            }

            if (!attemptScan(requestReader, errMsg, result, file_fd))
            {
                result.setErrorMsg(errMsg);
                LOGERROR(errMsg);
                sendResponse(socket_fd, result);
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