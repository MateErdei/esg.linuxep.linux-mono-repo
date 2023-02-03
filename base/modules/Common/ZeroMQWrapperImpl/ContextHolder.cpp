// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ContextHolder.h"

#include "ZeroMQWrapperException.h"
#include "Common/ZeroMQWrapperImpl/ContextCollection.h"
#include <zmq.h>

Common::ZeroMQWrapperImpl::ContextHolder::ContextHolder()
{
    m_context = ContextCollection::getInstance().createContext();
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
    ContextCollection::getInstance().closeContext(m_context);
    m_context = nullptr;
}
