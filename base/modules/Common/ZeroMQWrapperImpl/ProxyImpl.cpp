///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include "ProxyImpl.h"

#include "SocketHolder.h"
#include "SocketUtil.h"

#include <zmq.h>
#include <cassert>

using Common::ZeroMQWrapperImpl::ProxyImpl;

Common::ZeroMQWrapper::IProxyPtr Common::ZeroMQWrapper::createProxy(const std::string& frontend, const std::string& backend)
{
    return Common::ZeroMQWrapper::IProxyPtr(
                    new Common::ZeroMQWrapperImpl::ProxyImpl(frontend,backend)
                );
}

Common::ZeroMQWrapperImpl::ProxyImpl::ProxyImpl(const std::string &frontend, const std::string &backend)
    : m_frontendAddress(frontend),m_backendAddress(backend), m_threadStartedFlag(false)
{
}

Common::ZeroMQWrapperImpl::ProxyImpl::~ProxyImpl()
{
    // Will stop the thread eventually
    stop();
}

void Common::ZeroMQWrapperImpl::ProxyImpl::start()
{
    // Start thread
    assert(m_context.ctx() != nullptr);
    std::unique_lock<std::mutex> lock(m_threadStarted);
    m_thread = std::move(std::thread(&ProxyImpl::run,this));
    m_ensureThreadStarted.wait(lock,[this](){return m_threadStartedFlag;});
}

void Common::ZeroMQWrapperImpl::ProxyImpl::stop()
{
    m_context.reset();
    // Wait for thread to exit
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void ProxyImpl::announceThreadStarted()
{
    std::unique_lock<std::mutex> templock(m_threadStarted);
    m_threadStartedFlag = true;
    m_ensureThreadStarted.notify_all();
}

void Common::ZeroMQWrapperImpl::ProxyImpl::run()
{
    if (m_context.ctx() == nullptr)
    {
        announceThreadStarted();
        return;
    }

    SocketHolder xsub(m_context, ZMQ_XSUB);
    SocketHolder xpub(m_context, ZMQ_XPUB);

    const int timeoutMs = 1000;
    SocketUtil::setTimeout(xsub,timeoutMs);
    SocketUtil::setTimeout(xpub,timeoutMs);

    SocketUtil::listen(xsub, m_frontendAddress);
    SocketUtil::listen(xpub, m_backendAddress);

    announceThreadStarted();

    zmq_proxy(xsub.skt(), xpub.skt(), nullptr);
}

Common::ZeroMQWrapperImpl::ContextHolder & Common::ZeroMQWrapperImpl::ProxyImpl::ctx()
{
    return m_context;
}
