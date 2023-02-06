// Copyright 2023 Sophos Limited. All rights reserved.

#include "SocketCollection.h"

#include "ZeroMQWrapperException.h"

#include <zmq.h>

void* SocketCollection::createSocket(void* context,
                                     const int type)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto socket = zmq_socket(context, type);
    if (socket == nullptr)
    {
        throw Common::ZeroMQWrapperImpl::ZeroMQWrapperException("Failed to create socket");
    }
    m_zmqSockets.insert(socket);
    return socket;
}

void SocketCollection::closeSocket(void* socket)
{
    if (socket != nullptr)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_zmqSockets.erase(socket);
        zmq_close(socket);
    }
}

void SocketCollection::closeAll()
{
    for (auto socket: m_zmqSockets)
    {
        if (socket != nullptr)
        {
            zmq_close(socket);
        }
    }
}