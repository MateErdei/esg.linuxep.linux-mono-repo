//
// Created by pair on 06/06/18.
//

#include "ContextHolder.h"

#include <zmq.h>

Common::ZeroMQWrapperImpl::ContextHolder::ContextHolder()
{
    m_context = zmq_ctx_new();
}

Common::ZeroMQWrapperImpl::ContextHolder::~ContextHolder()
{
    zmq_ctx_destroy(m_context);
}

void *Common::ZeroMQWrapperImpl::ContextHolder::ctx()
{
    return m_context;
}
