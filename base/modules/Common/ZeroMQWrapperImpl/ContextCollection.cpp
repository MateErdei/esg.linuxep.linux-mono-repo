// Copyright 2023 Sophos Limited. All rights reserved.

#include "ContextCollection.h"

#include "ZeroMQWrapperException.h"

#include <zmq.h>

void* ContextCollection::createContext()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto context = zmq_ctx_new();
    if (context == nullptr)
    {
        throw Common::ZeroMQWrapperImpl::ZeroMQWrapperException("Unable to construct ZMQ Context");
    }
    m_zmqContexts.insert(context);
    return context;
}

void ContextCollection::closeContext(void* context)
{
    if (context != nullptr)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_zmqContexts.erase(context);
        zmq_ctx_term(context); // http://api.zeromq.org/4-2:zmq-ctx-term replaces destroy (deprecated)
    }
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
