// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "ThreatReporterServerConnectionThread.h"

#include "ThreatDetected.capnp.h"

#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include <capnp/serialize.h>
#include <common/FDUtils.h>
#include <common/SaferStrerror.h>
#include <scan_messages/ThreatDetected.h>

#include <cassert>
#include <optional>
#include <stdexcept>
#include <utility>

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace unixsocket;

ThreatReporterServerConnectionThread::ThreatReporterServerConnectionThread(datatypes::AutoFd& fd,
                                                                           std::shared_ptr<IMessageCallback> threatReportCallback,
                                                                           datatypes::ISystemCallWrapperSharedPtr sysCalls)
        : m_fd(std::move(fd))
        , m_threatReportCallback(std::move(threatReportCallback))
        , m_sysCalls(sysCalls)
{
    if (m_fd < 0)
    {
        throw std::runtime_error("Attempting to construct ThreatReporterServerConnectionThread with invalid socket fd");
    }

    if (!m_threatReportCallback)
    {
        throw std::runtime_error("Attempting to construct ThreatReporterServerConnectionThread with null callback");
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
 * Parse a detection.
 *
 * Placed in a function to ensure the MessageReader and view are deleted before the buffer might become invalid.
 *
 * @param proto_buffer
 * @param bytes_read
 * @return
 */
static scan_messages::ThreatDetected parseDetection(kj::Array<capnp::word>& proto_buffer, ssize_t& bytes_read)
{
    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    if (!requestReader.hasFilePath())
    {
        LOGERROR("Missing file path while parsing report ( size=" << bytes_read << " )");
    }
    return scan_messages::ThreatDetected(requestReader);
}

void ThreatReporterServerConnectionThread::run()
{
    setIsRunning(true);
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
            LOGFATAL("Terminated ThreatReporterServerConnectionThread with serialisation unimplemented exception: "
                         << ex.getDescription().cStr());
        }
        else
        {
            LOGERROR(
                "Terminated ThreatReporterServerConnectionThread with serialisation exception: "
                    << ex.getDescription().cStr());
        }
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Terminated ThreatReporterServerConnectionThread with exception: " << ex.what());
    }
    catch (...)
    {
        // Fatal since this means we have thrown something that isn't a subclass of std::exception
        LOGFATAL("Terminated ThreatReporterServerConnectionThread with unknown exception");
    }
    setIsRunning(false);
}

void ThreatReporterServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_fd));
    LOGDEBUG("Threat Reporter Server thread got connection " << socket_fd.fd());
    uint32_t buffer_size = 512;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    struct pollfd fds[] {
        { .fd = socket_fd.get(), .events = POLLIN, .revents = 0 }, // socket FD
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
    };
    bool loggedLengthOfZero = false;

    while (true)
    {
        auto ret = m_sysCalls->ppoll(fds, std::size(fds), nullptr, nullptr);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            LOGFATAL("Error from ppoll: " << common::safer_strerror(errno));
            break;
        }
        assert(ret > 0);

        if ((fds[1].revents & POLLERR) != 0)
        {
            LOGERROR("Closing Threat Reporter connection thread, error from notify pipe");
            break;
        }

        if ((fds[0].revents & POLLERR) != 0)
        {
            LOGERROR("Closing Threat Reporter connection thread, error from socket");
            break;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            LOGSUPPORT("Closing Threat Reporter connection thread");
            break;
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
            // read length
            int32_t length = unixsocket::readLength(socket_fd);
            if (length == -2)
            {
                LOGDEBUG("Threat Reporter connection thread closed: EOF");
                break;
            }
            else if (length < 0)
            {
                LOGERROR("Aborting Threat Reporter connection thread: failed to read length");
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
                LOGERROR("Aborting Threat Reporter connection thread: " << errno);
                break;
            }
            else if (bytes_read != length)
            {
                LOGERROR("Aborting Threat Reporter connection thread: failed to read entire message");
                break;
            }

            LOGDEBUG("Read capn of " << bytes_read);

            std::optional<scan_messages::ThreatDetected> threatDetectedOptional;
            try
            {
                threatDetectedOptional = parseDetection(proto_buffer, bytes_read);
            }
            catch (const std::exception& e)
            {
                LOGERROR("Aborting Threat Reporter connection thread: Failed to parse detection: " << e.what());
                break;
            }
            scan_messages::ThreatDetected detectionReader = std::move(threatDetectedOptional.value());

            // read fd
            datatypes::AutoFd file_fd(unixsocket::recv_fd(socket_fd));
            if (file_fd.get() < 0)
            {
                LOGERROR("Aborting Threat Reporter connection thread: failed to read fd");
                break;
            }
            LOGDEBUG("Managed to get file descriptor: " << file_fd.get());
            detectionReader.autoFd = std::move(file_fd);

            if (detectionReader.filePath.empty())
            {
                LOGERROR("Missing file path in detection report ( size=" << bytes_read << ")");
            }
            m_threatReportCallback->processMessage(std::move(detectionReader));
            // TODO: HANDLE ANY SOCKET ERRORS
        }
    }
}
