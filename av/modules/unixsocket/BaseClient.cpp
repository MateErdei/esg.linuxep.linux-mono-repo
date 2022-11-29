// Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "BaseClient.h"
// Package
#include "Logger.h"
// Std C++
#include <cassert>
// Std C
#include <tuple>
#include <sys/socket.h>
#include <sys/un.h>

using namespace unixsocket;

BaseClient::BaseClient(
    std::string socket_path,
    const duration_t& sleepTime,
    BaseClient::IStoppableSleeperSharedPtr sleeper)
    : m_socketPath(std::move(socket_path)),
      m_sleepTime(sleepTime),
      m_sleeper(std::move(sleeper))
{
    if (!m_sleeper)
    {
        m_sleeper = std::make_shared<IStoppableSleeper>();
    }
}

int unixsocket::BaseClient::attemptConnect()
{
    m_socket_fd.reset(socket(AF_UNIX, SOCK_STREAM, 0));
    assert(m_socket_fd.get() >= 0);

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    return ::connect(m_socket_fd.get(), reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
}

void unixsocket::BaseClient::connectWithRetries()
{
    connectWithRetries(m_socketPath);
}

void unixsocket::BaseClient::connectWithRetries(const std::string& socketName)
{
    const int MAX_CONN_RETRIES = 10;
    std::ignore = connectWithRetries(socketName, MAX_CONN_RETRIES);
}

bool BaseClient::connectWithRetries(const std::string& socketName, int max_retries)
{
    int count = 0;
    m_connectStatus = attemptConnect();

    while (m_connectStatus != 0)
    {
        if (++count >= max_retries)
        {
            LOGDEBUG("Reached total maximum number of connection attempts.");
            return false;
        }

        LOGDEBUG("Failed to connect to " << socketName << " - retrying after sleep");
        if (m_sleeper->stoppableSleep(m_sleepTime))
        {
            LOGINFO("Stop requested while connecting to "<< socketName);
            return false;
        }

        m_connectStatus = attemptConnect();
    }

    assert(m_connectStatus == 0);
    return true;
}

bool BaseClient::isConnected() const
{
    return m_connectStatus == 0;
}
