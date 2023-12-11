// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SafeStoreClient.h"

#include "Logger.h"

#include "common/CentralEnums.h"
#include "common/FDUtils.h"
#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/UnixSocketException.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <poll.h>
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
    BaseClient(std::move(socket_path), "SafeStoreClient", sleepTime, std::move(sleeper)), m_notifyPipe(notifyPipe)
{
    BaseClient::connectWithRetries();
}

void unixsocket::SafeStoreClient::sendQuarantineRequest(const scan_messages::ThreatDetected& detection)
{
    const std::string escapedPath = common::pathForLogging(detection.filePath);
    LOGDEBUG(m_name << " sending quarantine request to SafeStore for file: " << escapedPath);
    assert(m_socket_fd.valid());
    std::string dataAsString = detection.serialise();
    auto fd = detection.autoFd.get();

    Common::SystemCallWrapper::SystemCallWrapper systemCallWrapper;
    if (!writeLengthAndBuffer(systemCallWrapper, m_socket_fd, dataAsString))
    {
        throw unixsocket::UnixSocketException(LOCATION, m_name + " failed to write Threat Detection to socket: " + common::safer_strerror(errno));
    }

    auto ret = send_fd(m_socket_fd, fd);
    if (ret < 0)
    {
        throw unixsocket::UnixSocketException(LOCATION, m_name + " failed to write file descriptor to SafeStore socket: " + common::safer_strerror(errno));
    }

    LOGDEBUG(m_name << " sent quarantine request to SafeStore");
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
            throw unixsocket::UnixSocketException(LOCATION, "Closing " + m_name + ", error: " + common::safer_strerror(errno));
        }
        else if (active == 0)
        {
            throw unixsocket::UnixSocketException(LOCATION, m_name + " timed out waiting for response");
        }
        else
        {
            if (fds[1].revents & POLLIN)
            {
                // Still an exception because we expected a response
                throw unixsocket::UnixSocketException(LOCATION, m_name + " received thread stop notification");
            }
        }
        // read length
        ssize_t length = unixsocket::readLength(m_socket_fd);
        if (length == -2)
        {
            throw unixsocket::UnixSocketException(LOCATION, m_name + " got unexpected EOF");
        }
        else if (length < 0)
        {
            throw unixsocket::UnixSocketException(LOCATION, m_name + " failed to read length from socket");
        }
        else if (length == 0)
        {
            if (not loggedLengthOfZero)
            {
                LOGDEBUG(m_name << " ignoring length of zero / No new messages");
                loggedLengthOfZero = true;
            }
            continue;
        }

        // read capn proto
        if (static_cast<uint32_t>(length) > (buffer_size * sizeof(capnp::word)))
        {
            buffer_size = 1 + length / sizeof(capnp::word);
            proto_buffer = kj::heapArray<capnp::word>(buffer_size);
            loggedLengthOfZero = false; //NOLINT (clang-analyzer-deadcode.DeadStores)
        }

        ssize_t bytes_read = ::read(m_socket_fd, proto_buffer.begin(), length);
        if (bytes_read < 0)
        {
            throw unixsocket::UnixSocketException(LOCATION, m_name + " failed to get data: " + common::safer_strerror(errno));
        }
        else if (bytes_read != length)
        {
            throw unixsocket::UnixSocketException(LOCATION, m_name + " failed to read entire message");
        }
        auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

        capnp::FlatArrayMessageReader messageInput(view);
        Sophos::ssplav::QuarantineResponseMessage::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::QuarantineResponseMessage>();

        auto result = scan_messages::QuarantineResponse(requestReader).getResult();

        LOGDEBUG(m_name << " received quarantine result from SafeStore: "
            << (result == common::CentralEnums::QuarantineResult::SUCCESS ? "success" : "failure"));

        return result;
    }

    // Unreachable
    assert(false);
}
