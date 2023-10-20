// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "MetadataRescanServerConnectionThread.h"

#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/threatDetectorSocket/ThreatDetectedMessageUtils.h"
#include "unixsocket/UnixSocketException.h"

#include <capnp/serialize.h>

#include <cassert>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace unixsocket
{
    MetadataRescanServerConnectionThread::MetadataRescanServerConnectionThread(
        datatypes::AutoFd& fd,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory,
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  sysCalls,
        int maxIterations) :
        BaseServerConnectionThread("MetadataRescanServerConnectionThread"),
        m_socketFd(std::move(fd)),
        m_scannerFactory(std::move(scannerFactory)),
        m_sysCalls(std::move(sysCalls)),
        m_maxIterations(maxIterations)
    {
        if (m_socketFd < 0)
        {
            throw unixsocket::UnixSocketException(LOCATION, "Attempting to construct " + m_threadName + " with invalid socket fd");
        }

        if (m_scannerFactory.get() == nullptr)
        {
            throw unixsocket::UnixSocketException(LOCATION, "Attempting to construct " + m_threadName + " with null scanner factory");
        }
    }

    void MetadataRescanServerConnectionThread::run()
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
        std::ignore = s;
        assert(s == 0);

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
                LOGFATAL(
                    "Terminated " << m_threadName
                                  << " with serialisation unimplemented exception: " << ex.getDescription().cStr());
            }
            else
            {
                LOGERROR(
                    "Terminated " << m_threadName << " with serialisation exception: " << ex.getDescription().cStr());
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

    bool MetadataRescanServerConnectionThread::sendResponse(
        datatypes::AutoFd& socket_fd,
        const scan_messages::MetadataRescanResponse& response)
    {
        LOGDEBUG("Sending response to metadata rescan request");
        const auto bytesWritten =
            ::send(socket_fd, &response, sizeof(scan_messages::MetadataRescanResponse), MSG_NOSIGNAL);
        if (bytesWritten != sizeof(scan_messages::MetadataRescanResponse))
        {
            LOGWARN("Exiting " << m_threadName);
            return false;
        }
        return true;
    }

    void MetadataRescanServerConnectionThread::inner_run()
    {
        datatypes::AutoFd socket_fd(std::move(m_socketFd));
        LOGDEBUG(m_threadName << " got connection " << socket_fd.fd());
        uint32_t buffer_size = 256;
        auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

        struct pollfd fds[]
        {
            // clang-format off
            { .fd = socket_fd.get(), .events = POLLIN, .revents = 0 }, // socket FD
            { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
            // clang-format on
        };
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

            auto ret = m_sysCalls->ppoll(fds, std::size(fds), nullptr, nullptr);
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
                ssize_t bytes_read;
                std::string errMsg;
                if (!readCapnProtoMsg(
                        m_sysCalls,
                        length,
                        buffer_size,
                        proto_buffer,
                        socket_fd,
                        bytes_read,
                        loggedLengthOfZero,
                        errMsg,
                        readTimeout_))
                {
                    sendResponse(socket_fd, scan_messages::MetadataRescanResponse::failed);
                    LOGERROR(m_threadName << ": " << errMsg);
                    break;
                }
                LOGDEBUG(m_threadName << " read capn of " << bytes_read);

                auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));
                const auto request = scan_messages::MetadataRescan::Deserialise(view);

                LOGDEBUG(
                    m_threadName << " received a metadata rescan request of filePath="
                                 << common::escapePathForLogging(request.filePath)
                                 << ", threatType=" << request.threat.type << ", threatName=" << request.threat.name
                                 << ", threatSHA256=" << request.threat.sha256 << ", SHA256=" << request.sha256);

                if (!scanner)
                {
                    // All settings set to true to maximise detections for rescans, but likely they don't have an effect
                    // on metadata rescans
                    scanner = m_scannerFactory->createScanner(true, true, true);
                }
                if (!scanner)
                {
                    throw unixsocket::UnixSocketException(LOCATION, m_threadName + " failed to create scanner");
                }

                const auto result = scanner->metadataRescan(request);

                if (!sendResponse(socket_fd, result))
                {
                    break;
                }
            }
        }
    }
} // namespace unixsocket