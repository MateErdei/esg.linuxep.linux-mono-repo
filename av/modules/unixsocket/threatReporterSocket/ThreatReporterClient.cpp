// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ThreatReporterClient.h"

#include "common/SaferStrerror.h"

#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"
#include "unixsocket/UnixSocketException.h"

#include "scan_messages/ThreatDetected.h"

#include <string>
#include <cassert>
#include <sstream>

unixsocket::ThreatReporterClientSocket::ThreatReporterClientSocket(
    std::string socket_path,
    const duration_t& sleepTime,
    IStoppableSleeperSharedPtr sleeper)
    : BaseClient(std::move(socket_path), "ThreatReporterClient", sleepTime, std::move(sleeper))
{
    connectWithRetries();
}

void unixsocket::ThreatReporterClientSocket::sendThreatDetection(const scan_messages::ThreatDetected& detection)
{
    assert(m_socket_fd >= 0);
    std::string dataAsString = detection.serialise();
    int fd = detection.autoFd.get();

    try
    {
        if (!writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
            std::stringstream errMsg;
            errMsg << m_name << " failed to write to socket [" << common::safer_strerror(errno) << "]";
            throw unixsocket::UnixSocketException(LOCATION, errMsg.str());
        }

        if (send_fd(m_socket_fd, fd) < 0)
        {
            throw unixsocket::UnixSocketException(LOCATION, m_name + " failed to write file descriptor to socket");
        }
    }
    catch (unixsocket::EnvironmentInterruption& e)
    {
        LOGERROR(m_name << " failed to write to socket. Exception caught: " << e.what());
    }
}