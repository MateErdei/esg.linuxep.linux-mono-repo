// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SafeStoreServerConnectionThread.h"

#include "Logger.h"
#include "ThreatDetected.capnp.h"

#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "safestore/SafeStoreTelemetryConsts.h"
#include "scan_messages/QuarantineResponse.h"
#include "scan_messages/ThreatDetected.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/UnixSocketException.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <poll.h>
#include <unistd.h>

#include <capnp/serialize.h>

#include <cassert>
#include <optional>
#include <stdexcept>
#include <utility>

using namespace unixsocket;

SafeStoreServerConnectionThread::SafeStoreServerConnectionThread(
    datatypes::AutoFd& fd,
    std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager,
    Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  sysCalls) :
    BaseServerConnectionThread("SafeStoreServerConnectionThread"),
    m_fd(std::move(fd)),
    m_quarantineManager(std::move(quarantineManager)),
    m_sysCalls(std::move(sysCalls))
{
    if (m_fd < 0)
    {
        throw unixsocket::UnixSocketException(LOCATION, "Attempting to construct " + m_threadName + " with invalid socket fd");
    }
}

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
    Sophos::ssplav::ThreatDetected::Reader requestReader = messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    if (!requestReader.hasFilePath())
    {
        LOGERROR("Missing file path while parsing report ( size=" << bytes_read << " )");
    }
    return scan_messages::ThreatDetected::deserialise(requestReader);
}

std::string getResponse(common::CentralEnums::QuarantineResult quarantineResult)
{
    std::shared_ptr<scan_messages::QuarantineResponse> request =
        std::make_shared<scan_messages::QuarantineResponse>(quarantineResult);
    std::string dataAsString = request->serialise();

    return dataAsString;
}

void SafeStoreServerConnectionThread::run()
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
            LOGFATAL(
                "Terminated " << m_threadName
                              << " with serialisation unimplemented exception: " << ex.getDescription().cStr());
        }
        else
        {
            LOGERROR("Terminated " << m_threadName << " with serialisation exception: " << ex.getDescription().cStr());
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
    setIsRunning(false);
}

void SafeStoreServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_fd));
    LOGDEBUG(m_threadName << " got connection " << socket_fd.fd());
    uint32_t buffer_size = 512;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    struct pollfd fds[]
    {
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
            LOGERROR("Closing " << m_threadName << ", error from socket");
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
                LOGERROR("Aborting " << m_threadName << ": failed to read length");
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
            if (static_cast<uint32_t>(length) > (buffer_size * sizeof(capnp::word)))
            {
                buffer_size = 1 + length / sizeof(capnp::word);
                proto_buffer = kj::heapArray<capnp::word>(buffer_size);
                loggedLengthOfZero = false;
            }

            ssize_t bytes_read = ::read(socket_fd, proto_buffer.begin(), length);
            if (bytes_read < 0)
            {
                LOGERROR("Aborting " << m_threadName << ": " << errno);
                break;
            }
            else if (bytes_read != length)
            {
                LOGERROR("Aborting " << m_threadName << ": failed to read entire message");
                break;
            }

            LOGDEBUG(m_threadName << " read capn of " << bytes_read);
            std::optional<scan_messages::ThreatDetected> threatDetectedOptional;

            try
            {
                threatDetectedOptional = parseDetection(proto_buffer, bytes_read);
            }
            catch (const std::exception& e)
            {
                LOGERROR("Aborting " << m_threadName << ": failed to parse detection");
                break;
            }

            scan_messages::ThreatDetected threatDetected = std::move(threatDetectedOptional.value());

            // read fd
            datatypes::AutoFd file_fd(unixsocket::recv_fd(socket_fd));
            if (file_fd.get() < 0)
            {
                LOGERROR("Aborting " << m_threadName << ": failed to read fd");
                break;
            }
            LOGDEBUG(m_threadName << " managed to get file descriptor: " << file_fd.get());
            threatDetected.autoFd = std::move(file_fd);

            if (threatDetected.filePath.empty())
            {
                LOGERROR(m_threadName << " missing file path in detection report ( size=" << bytes_read << ")");
            }
            const std::string escapedPath = common::escapePathForLogging(threatDetected.filePath);
            LOGDEBUG(
                "Received Threat:\n  File path: "
                << escapedPath << "\n  Threat ID: " << threatDetected.threatId
                << "\n  Threat type: " << threatDetected.threatType << "\n  Threat name: " << threatDetected.threatName
                << "\n  Threat SHA256: " << threatDetected.threatSha256 << "\n  SHA256: " << threatDetected.sha256
                << "\n  File descriptor: " << threatDetected.autoFd.get());

            common::CentralEnums::QuarantineResult quarantineResult = m_quarantineManager->quarantineFile(
                threatDetected.filePath,
                threatDetected.threatId,
                threatDetected.threatType,
                threatDetected.threatName,
                threatDetected.threatSha256,
                threatDetected.sha256,
                threatDetected.correlationId,
                std::move(threatDetected.autoFd));

            switch (quarantineResult)
            {
                case common::CentralEnums::QuarantineResult::SUCCESS:
                {
                    Common::Telemetry::TelemetryHelper::getInstance().increment(
                        safestore::telemetrySafeStoreQuarantineSuccess, 1ul);
                    break;
                }
                case common::CentralEnums::QuarantineResult::NOT_FOUND:
                {
                    Common::Telemetry::TelemetryHelper::getInstance().increment(
                        safestore::telemetrySafeStoreQuarantineFailure, 1ul);
                    break;
                }
                case common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE:
                {
                    Common::Telemetry::TelemetryHelper::getInstance().increment(
                        safestore::telemetrySafeStoreUnlinkFailure, 1ul);
                    break;
                }
                default:
                {
                    break;
                }
            }

            std::string serialised_result = getResponse(quarantineResult);

            try
            {
                if (!writeLengthAndBuffer(socket_fd, serialised_result))
                {
                    LOGWARN(m_threadName << " failed to write result to unix socket");
                    break;
                }
            }
            catch (unixsocket::environmentInterruption& e)
            {
                LOGWARN("Exiting " << m_threadName << ": " << e.what());
                break;
            }
        }
    }
}
