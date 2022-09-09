// Copyright 2022, Sophos Limited.  All rights reserved.

#include "UpdateCompleteClientSocketThread.h"

#include "../Logger.h"

#include "common/SaferStrerror.h"

#include <utility>

#include <poll.h>

unixsocket::updateCompleteSocket::UpdateCompleteClientSocketThread::UpdateCompleteClientSocketThread(
    std::string socket_path,
    unixsocket::updateCompleteSocket::UpdateCompleteClientSocketThread::IUpdateCompleteCallbackPtr callback,
    struct timespec reconnectInterval)
    :
    BaseClient(std::move(socket_path)),
    m_callback(std::move(callback)),
    m_reconnectInterval(reconnectInterval)
{
}

void unixsocket::updateCompleteSocket::UpdateCompleteClientSocketThread::run()
{
    const int SOCKET = 0;

    struct pollfd fds[] {
        { .fd = -1, .events = POLLIN, .revents = 0 }, // socket FD
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
    };

    const struct timespec* timeout = &m_reconnectInterval;

    // Try to connect if we don't have a valid connection
    connectIfNotConnected();

    // Only announce start once we've tried once to connect
    announceThreadStarted();

    while (true)
    {
        // Try to connect if we don't have a valid connection
        connectIfNotConnected();

        // If we have a connection the wait for events, otherwise timeout for reconnection
        if (connected())
        {
            fds[SOCKET].fd = m_socket_fd.get();
            timeout = nullptr;
        }
        else
        {
            fds[SOCKET].fd = -1;
            timeout = &m_reconnectInterval;
        }

        auto ret = ::ppoll(fds, std::size(fds), timeout, nullptr);

        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            LOGFATAL("Error from ppoll: " << common::safer_strerror(errno));
            return;
        }

        else if (ret > 0)
        {
            if ((fds[1].revents & POLLIN) != 0)
            {
                while (m_notifyPipe.notified())
                {
                }
                return;
            }

            if ((fds[SOCKET].revents & POLLIN) != 0)
            {
                char buffer[1];
                auto charsRead = ::read(m_socket_fd.get(), buffer, 1);
                if (charsRead == 1 && buffer[0] == '1')
                {
                    m_callback->updateComplete();
                }
                else
                {
                    m_socket_fd.close();
                }
            }
            else if (fds[SOCKET].revents > 0)
            {
                // Some kind of error - POLLERR, POLLHUP, POLLNVAL
                m_socket_fd.close();
                m_connectStatus = -1;
            }
        }
    }
}

bool unixsocket::updateCompleteSocket::UpdateCompleteClientSocketThread::connected()
{
    return m_socket_fd.valid() && m_connectStatus == 0;
}

void unixsocket::updateCompleteSocket::UpdateCompleteClientSocketThread::connectIfNotConnected()
{
    if (!connected())
    {
        m_connectStatus = attemptConnect();
    }
}
