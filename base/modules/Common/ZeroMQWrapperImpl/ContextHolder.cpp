/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ContextHolder.h"

#include "ZeroMQWrapperException.h"

#include <zmq.h>

Common::ZeroMQWrapperImpl::ContextHolder::ContextHolder()
{
    m_context = zmq_ctx_new();
    if (m_context == nullptr)
    {
        throw ZeroMQWrapperException("Unable to construct ZMQ Context");
    }
}

Common::ZeroMQWrapperImpl::ContextHolder::~ContextHolder()
{
    reset();
}

void* Common::ZeroMQWrapperImpl::ContextHolder::ctx()
{
    return m_context;
}

void Common::ZeroMQWrapperImpl::ContextHolder::reset()
{
    if (m_context != nullptr)
    {
        zmq_ctx_term(m_context); // http://api.zeromq.org/4-2:zmq-ctx-term replaces destroy (deprecated)
        m_context = nullptr;
    }
}
