// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreClient.h"

#include "scan_messages/ThreatDetected.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include <cassert>
#include <sstream>
#include <string>

#include <sys/socket.h>

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
//
//void unixsocket::SafeStoreClient::waitForResponse()
//{
//    struct pollfd fds[] {
//        { .fd = m_socket.socketFd(), .events = POLLIN, .revents = 0 },
//            { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
//    };
//
//    while (true)
//    {
//        auto ret = ::ppoll(fds, std::size(fds), nullptr, nullptr);
//
//        if (ret < 0)
//        {
//            if (errno == EINTR)
//            {
//                continue;
//            }
//
//            LOGERROR("Error from ppoll: " << common::safer_strerror(errno));
//            throw ClientSocketException("Error while waiting for scan response");
//        }
//
//        else if (ret > 0)
//        {
//
//            break;
//        }
//    }
//}
