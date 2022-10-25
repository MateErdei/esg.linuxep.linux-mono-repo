// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreClient.h"

#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "common/SaferStrerror.h"
#include <common/FDUtils.h>
#include <capnp/serialize.h>

#include <cassert>
#include <sstream>
#include <string>

#include <sys/socket.h>

#include <poll.h>
#include <unistd.h>
unixsocket::SafeStoreClient::SafeStoreClient(
    std::string socket_path,
    const duration_t& sleepTime,
    IStoppableSleeperSharedPtr sleeper) :
    BaseClient(std::move(socket_path), sleepTime, std::move(sleeper))
{
    BaseClient::connectWithRetries("SafeStore");
}

void unixsocket::SafeStoreClient::sendQuarantineRequest(const scan_messages::ThreatDetected& detection)
{
    LOGDEBUG("Sending quarantine request for file: " << detection.filePath);
    assert(m_socket_fd.valid());
    std::string dataAsString = detection.serialise();
    auto fd = detection.autoFd.get();

    try
    {
        if (!writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
            std::stringstream errMsg;
            errMsg << "Failed to write Threat Detection to socket [" << errno << "]";
            throw std::runtime_error(errMsg.str());
        }

        auto ret = send_fd(m_socket_fd, fd);
        if (ret < 0)
        {
            throw std::runtime_error("Failed to write file descriptor to SafeStore socket");
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGWARN("Failed to write to SafeStore socket. Exception caught: " << e.what());
    }
}

scan_messages::QuarantineResult unixsocket::SafeStoreClient::waitForResponse()
{
    uint32_t buffer_size = 512;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);


    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = FDUtils::addFD(&readFDs, m_socket_fd, max);
    bool loggedLengthOfZero = false;
    struct timespec ts;
    ts.tv_sec = 60;
    ts.tv_nsec = 0;
    while (true)
    {
        fd_set tempRead = readFDs;

        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, &ts, nullptr);

        if (activity < 0)
        {
            LOGERROR("Closing AV connection thread, error: " << errno);
            break;
        }

        LOGINFO(m_socket_fd.get());
        assert(FDUtils::fd_isset(m_socket_fd, &tempRead));
        // read length
        int32_t length = unixsocket::readLength(m_socket_fd);
        if (length == -2)
        {
            LOGDEBUG("av SafeStore connection closed: EOF");
            break;
        }
        else if (length < 0)
        {
            LOGERROR("Aborting av SafeStore connection : failed to read length");
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

        ssize_t bytes_read = ::read(m_socket_fd, proto_buffer.begin(), length);
        if (bytes_read < 0)
        {
            LOGERROR("Aborting SafeStore connection thread: " << errno);
            break;
        }
        else if (bytes_read != length)
        {
            LOGERROR("Aborting SafeStore connection thread: failed to read entire message");
            break;
        }
        auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

        capnp::FlatArrayMessageReader messageInput(view);
        Sophos::ssplav::QuarantineResponseMessage::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::QuarantineResponseMessage>();
        LOGINFO("Got response from ss");
        return scan_messages::QuarantineResponse(requestReader).getResult();
        }

        throw std::runtime_error("Failed to get response from safestore");
}


