// Copyright 2023 Sophos Limited. All rights reserved.

#include "ContextCollection.h"

#include "ZeroMQWrapperException.h"

#include <chrono>
#include <zmq.h>

using namespace std::chrono_literals;

void* ContextCollection::createContext()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait_for(lock, 1s);
    auto context = zmq_ctx_new();
    if (context == nullptr)
    {
        throw Common::ZeroMQWrapperImpl::ZeroMQWrapperException("Unable to construct ZMQ Context");
    }
    m_zmqContexts.push_back(context);
    m_cond.notify_one();
    return context;
}

void ContextCollection::closeAll()
{
    for (const auto context: m_zmqContexts)
    {
        if (context != nullptr)
        {
            zmq_ctx_term(context);
        }
    }
}
