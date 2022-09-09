// Copyright 2022, Sophos Limited.  All rights reserved.

#include "UpdateCompleteServerSocket.h"

using namespace unixsocket::updateCompleteSocket;

UpdateCompleteServerSocket::UpdateCompleteServerSocket(
    const sophos_filesystem::path& path,
    mode_t mode) :
    BaseServerSocket(path, mode)
{
}

bool UpdateCompleteServerSocket::handleConnection(datatypes::AutoFd& fd)
{
    std::scoped_lock lock(m_connectionsLock);
    m_connections.push_back(std::move(fd));
    return false;
}
void UpdateCompleteServerSocket::publishUpdateComplete()
{
    std::scoped_lock lock(m_connectionsLock);
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

bool UpdateCompleteServerSocket::trySendUpdateComplete(datatypes::AutoFd& fd)
{
    auto ret = ::write(fd.get(), "1", 1);
    return ret == 1;
}

void UpdateCompleteServerSocket::killThreads()
{
    std::scoped_lock lock(m_connectionsLock);
    m_connections.clear();
}

using ConnectionVector = UpdateCompleteServerSocket::ConnectionVector;
ConnectionVector::size_type UpdateCompleteServerSocket::clientCount() const
{
    std::scoped_lock lock(m_connectionsLock);
    return m_connections.size();
}
