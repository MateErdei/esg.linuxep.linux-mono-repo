/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporterClient.h"

#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"
#include "scan_messages/ThreatDetected.h"
#include <ScanResponse.capnp.h>

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <sys/socket.h>
#include <sys/un.h>
#include <sstream>

#define MAX_CONN_RETRIES 10



unixsocket::ThreatReporterClientSocket::ThreatReporterClientSocket(std::string socket_path, const struct timespec& sleepTime)
    : m_socketPath(std::move(socket_path))
    , m_sleepTime(sleepTime)
{
    connect();
}

void unixsocket::ThreatReporterClientSocket::connect()
{
    int ret = attemptConnect();
    int reconnectionCounts = 0;
    while (ret != 0)
    {
        if (++reconnectionCounts >= MAX_CONN_RETRIES)
        {
            LOGDEBUG("Reached total maximum number of connection attempts.");
            return;
        }

        LOGDEBUG("Failed to connect to Threat reporter - retrying after sleep");
        nanosleep(&m_sleepTime, nullptr);

        ret = attemptConnect();
    }

    assert(ret == 0);
    LOGDEBUG("Successfully connected to Threat Reporter");
}
int unixsocket::ThreatReporterClientSocket::attemptConnect()
{
    m_socket_fd.reset(socket(AF_UNIX, SOCK_STREAM, 0));
    assert(m_socket_fd >= 0);

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    return ::connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
}

void unixsocket::ThreatReporterClientSocket::sendThreatDetection(const scan_messages::ThreatDetected& detection)
{
    assert(m_socket_fd >= 0);
    std::string dataAsString = detection.serialise();

    try
    {
        if (! writeLengthAndBuffer(m_socket_fd, dataAsString))
        {
                std::stringstream errMsg;
                errMsg << "Failed to write Process Control Client to socket [" << errno << "]";
                throw std::runtime_error(errMsg.str());
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGERROR("Failed to write Threat Report Client to socket. Exception caught: " << e.what());
    }
}

