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
    m_zmqContexts.push_back(context);
    m_contextCollectionCondition.notify_one();
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
