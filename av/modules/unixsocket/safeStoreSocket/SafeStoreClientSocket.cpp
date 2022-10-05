// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreClientSocket.h"

#include "scan_messages/ThreatDetected.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include <cassert>
#include <sstream>
#include <string>

#include <sys/socket.h>

unixsocket::SafeStoreClientSocket::SafeStoreClientSocket(
    std::string socket_path,
    const struct timespec& sleepTime) :
    BaseClient(std::move(socket_path), sleepTime)
{
    BaseClient::connectWithRetries("SafeStore");
}

void unixsocket::SafeStoreClientSocket::sendQuarantineRequest(const scan_messages::ThreatDetected& detection)
{
    assert(m_socket_fd >= 0);
    std::string dataAsString = detection.serialise();
    auto fd = detection.getFd();

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
        LOGERROR("Failed to write to SafeStore socket. Exception caught: " << e.what());
    }
}
