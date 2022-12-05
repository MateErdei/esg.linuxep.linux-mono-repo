// Copyright 2022 Sophos Limited. All rights reserved.

#include "SafeStoreClient.h"

#include "Logger.h"

#include "common/CentralEnums.h"
#include "common/FDUtils.h"
#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "unixsocket/SocketUtils.h"

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <capnp/serialize.h>

#include <cassert>
#include <sstream>
#include <string>

unixsocket::SafeStoreClient::SafeStoreClient(
    std::string socket_path,
    Common::Threads::NotifyPipe& notifyPipe,
    const duration_t& sleepTime,
    IStoppableSleeperSharedPtr sleeper) :
    BaseClient(std::move(socket_path), sleepTime, std::move(sleeper)), m_notifyPipe(notifyPipe)
{
    BaseClient::connectWithRetries("SafeStore");
    if (isConnected())
    {
        LOGDEBUG("Connected to SafeStore");
    }
}

void unixsocket::SafeStoreClient::sendQuarantineRequest(const scan_messages::ThreatDetected& detection)
{
    const std::string escapedPath = common::escapePathForLogging(detection.filePath);
    LOGDEBUG("Sending quarantine request to SafeStore for file: " << escapedPath);
    assert(m_socket_fd.valid());
    std::string dataAsString = detection.serialise();
    auto fd = detection.autoFd.get();

    if (!writeLengthAndBuffer(m_socket_fd, dataAsString))
    {
        throw std::runtime_error("Failed to write Threat Detection to socket: " + common::safer_strerror(errno));
    }

    auto ret = send_fd(m_socket_fd, fd);
    if (ret < 0)
    {
        throw std::runtime_error("Failed to write file descriptor to SafeStore socket");
    }

    LOGDEBUG("Sent quarantine request to SafeStore");
}

common::CentralEnums::QuarantineResult unixsocket::SafeStoreClient::waitForResponse()
{
    uint32_t buffer_size = 512;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    struct pollfd fds[]
    {
        // clang-format off
        { .fd = m_socket_fd.get(), .events = POLLIN, .revents = 0 },
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 }
        // clang-format on
    };
    bool loggedLengthOfZero = false;
    timespec timeout{ .tv_sec = 900, .tv_nsec = 0 };
    while (true)
    {
        int active = ::ppoll(fds, std::size(fds), &timeout, nullptr);
        if (active < 0)
        {
            throw std::runtime_error("Closing SafeStore connection thread, error: " + common::safer_strerror(errno));
        }
        else if (active == 0)
        {
            throw std::runtime_error("Timed out waiting for response");
        }
        else
        {
            if (fds[1].revents & POLLIN)
            {
                // Still an exception because we expected a response
                throw std::runtime_error("Received thread stop notification");
            }
        }
        // read length
        ssize_t length = unixsocket::readLength(m_socket_fd);
        if (length == -2)
        {
            throw std::runtime_error("Unexpected EOF");
        }
        else if (length < 0)
        {
            throw std::runtime_error("Failed to read length from socket");
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

        ssize_t bytes_read = ::read(m_socket_fd, proto_buffer.begin(), length);
        if (bytes_read < 0)
        {
            throw std::runtime_error("Failed to get data: " + common::safer_strerror(errno));
        }
        else if (bytes_read != length)
        {
            throw std::runtime_error("Failed to read entire message");
        }
        auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

        capnp::FlatArrayMessageReader messageInput(view);
        Sophos::ssplav::QuarantineResponseMessage::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::QuarantineResponseMessage>();

        auto result = scan_messages::QuarantineResponse(requestReader).getResult();

        LOGDEBUG(
            "Received quarantine result from SafeStore: "
            << (result == common::CentralEnums::QuarantineResult::SUCCESS ? "success" : "failure"));

        return result;
    }

    // Unreachable
    assert(false);
}
