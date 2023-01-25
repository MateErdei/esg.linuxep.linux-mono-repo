// Copyright 2022-2023 Sophos Limited. All rights reserved.

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
    std::string name,
    const duration_t& sleepTime,
    BaseClient::IStoppableSleeperSharedPtr sleeper)
    : m_socketPath(std::move(socket_path)),
      m_sleepTime(sleepTime),
      m_name(std::move(name)),
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

bool BaseClient::connectWithRetries(int max_retries)
{
    int count = 0;
    m_connectStatus = attemptConnect();
    bool connectRetryLogged = false;

    while (m_connectStatus != 0)
    {
        if (++count >= max_retries)
        {
            LOGDEBUG(m_name << " reached the maximum number of attempts");
            return false;
        }

        if (!connectRetryLogged)
        {
            LOGDEBUG(m_name << " failed to connect to " << m_socketPath << " - retrying upto " << max_retries << " times with a sleep of "
                                             << std::chrono::duration_cast<std::chrono::seconds>(m_sleepTime).count() << "s");
            connectRetryLogged = true;
        }
        if (m_sleeper->stoppableSleep(m_sleepTime))
        {
            LOGINFO(m_name << " received stop request while connecting");
            return false;
        }

        m_connectStatus = attemptConnect();
    }

    assert(m_connectStatus == 0);
    LOGDEBUG(m_name << " connected");
    return true;
}

bool BaseClient::isConnected() const
{
    return m_connectStatus == 0;
}
