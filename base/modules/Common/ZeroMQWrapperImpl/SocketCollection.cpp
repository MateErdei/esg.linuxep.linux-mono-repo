// Copyright 2023 Sophos Limited. All rights reserved.

#include "SocketCollection.h"

#include "ZeroMQWrapperException.h"

#include <chrono>
#include <zmq.h>

using namespace std::chrono_literals;

void* SocketCollection::createSocket(void* context,
                          const int type)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait_for(lock, 1s);
    auto socket = zmq_socket(context, type);
    if (socket == nullptr)
    {
        throw Common::ZeroMQWrapperImpl::ZeroMQWrapperException("Failed to create socket");
    }
    m_zmqSockets.push_back(socket);
    m_cond.notify_one();
    return socket;
}

void SocketCollection::closeAll()
{
    for (const auto socket: m_zmqSockets)
    {
        if (socket != nullptr)
        {
            zmq_close(socket);
        }
    }
}
