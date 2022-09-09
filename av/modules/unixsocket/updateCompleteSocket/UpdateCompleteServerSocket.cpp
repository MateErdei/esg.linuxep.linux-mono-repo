// Copyright 2022, Sophos Limited.  All rights reserved.

#include "UpdateCompleteServerSocket.h"
bool unixsocket::updateCompleteSocket::UpdateCompleteServerSocket::handleConnection(datatypes::AutoFd& fd)
{
    m_connections.push_back(std::move(fd));
    return false;
}
void unixsocket::updateCompleteSocket::UpdateCompleteServerSocket::publishUpdateComplete()
{
    for (auto it = m_connections.begin(); it != m_connections.end() ;)
    {
        // try to send to connection
        if (trySendUpdateComplete(*it))
        {
            ++it;
        }
        else
        {
            // Connection broken
            it = m_connections.erase(it);
        }
    }
}

bool unixsocket::updateCompleteSocket::UpdateCompleteServerSocket::trySendUpdateComplete(datatypes::AutoFd& fd)
{
    auto ret = ::write(fd.get(), "1", 1);
    return ret == 1;
}
