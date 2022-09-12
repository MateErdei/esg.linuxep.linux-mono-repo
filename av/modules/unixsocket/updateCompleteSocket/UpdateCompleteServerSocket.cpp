// Copyright 2022, Sophos Limited.  All rights reserved.

#include "UpdateCompleteServerSocket.h"

#include "../Logger.h"

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
    LOGINFO("New connection on "<<fd.get() << " to be notified about SUSI updates");
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
            LOGINFO("Send SUSI updated to " << (*it).get());
            ++it;
        }
        else
        {
            // Connection broken
            LOGINFO("Connection " << (*it).get() << " has disconnected");
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

void UpdateCompleteServerSocket::updateComplete()
{
    publishUpdateComplete();
}
