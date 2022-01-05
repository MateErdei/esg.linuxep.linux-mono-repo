/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessControllerClient.h"

#include "unixsocket/SocketUtils.h"
#include "unixsocket/Logger.h"
#include <ScanResponse.capnp.h>

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <utility>
#include <sstream>

#include <sys/socket.h>
#include <sys/un.h>

#define MAX_CONN_RETRIES 10

int unixsocket::ProcessControllerClientSocket::attemptConnect()
{
    m_socket_fd.reset(socket(AF_UNIX, SOCK_STREAM, 0));
    assert(m_socket_fd >= 0);

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    return ::connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
}

void unixsocket::ProcessControllerClientSocket::connect()
{
    int count = 0;
    m_connectStatus = attemptConnect();

    while (m_connectStatus != 0)
    {
        if (++count >= MAX_CONN_RETRIES)
        {
            LOGDEBUG("Reached total maximum number of connection attempts.");
            return;
        }

        LOGDEBUG("Failed to connect to Sophos Threat Detector Controller - retrying after sleep");
        nanosleep(&m_sleepTime, nullptr);

        m_connectStatus = attemptConnect();
    }

    assert(m_connectStatus == 0);
}

unixsocket::ProcessControllerClientSocket::ProcessControllerClientSocket(std::string socket_path, const struct timespec& sleepTime)
        : m_socketPath(std::move(socket_path))
        , m_sleepTime(sleepTime)
{
    connect();
}

void unixsocket::ProcessControllerClientSocket::sendProcessControlRequest(const scan_messages::ProcessControlSerialiser& processControl)
{
    if (m_connectStatus == -1)
    {
        LOGWARN("Invalid connection status: message was not sent");
        return;
    }

    assert(m_socket_fd >= 0);
    std::string dataAsString = processControl.serialise();

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
        LOGERROR("Failed to write Process Control Request to socket. Exception caught: " << e.what());
    }
}
