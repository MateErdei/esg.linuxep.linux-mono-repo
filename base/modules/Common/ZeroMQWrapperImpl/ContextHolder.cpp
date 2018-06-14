//
// Created by pair on 06/06/18.
//

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

void *Common::ZeroMQWrapperImpl::ContextHolder::ctx()
{
    return m_context;
}

void Common::ZeroMQWrapperImpl::ContextHolder::reset()
{
    if (m_context != nullptr)
    {
        zmq_ctx_destroy(m_context);
        m_context = nullptr;
    }
}
