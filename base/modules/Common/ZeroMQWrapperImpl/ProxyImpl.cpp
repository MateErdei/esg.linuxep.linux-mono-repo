/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProxyImpl.h"

#include "SocketUtil.h"
#include "ZeroMQTimeoutException.h"
#include "ZeroMQWrapperException.h"

#include <cassert>
#include <zmq.h>

using Common::ZeroMQWrapperImpl::ProxyImpl;

Common::ZeroMQWrapperImpl::ProxyImpl::ProxyImpl(
    const std::string& frontend,
    const std::string& backend,
    ContextHolderSharedPtr context) :
    m_frontendAddress(frontend),
    m_backendAddress(backend),
    m_controlAddress("inproc://PubSubControl"),
    m_context(std::move(context)),
    m_threadStartedFlag(false),
    m_controlPub(m_context, ZMQ_PUSH),
    m_captureZMQSocket(nullptr)
{
    SocketUtil::listen(m_controlPub, m_controlAddress);
}

Common::ZeroMQWrapperImpl::ProxyImpl::~ProxyImpl()
{
    // Will stop the thread eventually
    stopProxy();
}

void Common::ZeroMQWrapperImpl::ProxyImpl::start()
{
    // Start thread
    assert(m_context.get() != nullptr);
    // cppcheck-suppress assertWithSideEffect
    assert(m_context->ctx() != nullptr);
    std::unique_lock<std::mutex> lock(m_threadStarted);
    m_thread = std::thread(&ProxyImpl::run, this);
    m_ensureThreadStarted.wait(lock, [this]() { return m_threadStartedFlag; });
}

void Common::ZeroMQWrapperImpl::ProxyImpl::stopProxy()
{
    // ensure that we have not already stopped
    if (m_thread.joinable())
    {
        std::vector<std::string> terminate = { "TERMINATE" };
        try
        {
            SocketUtil::write(m_controlPub, terminate);
        }
        catch (const Common::ZeroMQWrapperImpl::ZeroMQTimeoutException&)
        {
            std::terminate();
        }
        catch (const Common::ZeroMQWrapperImpl::ZeroMQWrapperException&)
        {
            std::terminate();
        }
        // Wait for thread to exit
        m_thread.join();
    }
}

void Common::ZeroMQWrapperImpl::ProxyImpl::stop()
{
    stopProxy();
}

void ProxyImpl::announceThreadStarted()
{
    std::unique_lock<std::mutex> templock(m_threadStarted);
    m_threadStartedFlag = true;
    m_ensureThreadStarted.notify_all();
}

void Common::ZeroMQWrapperImpl::ProxyImpl::run()
{
    if (m_context->ctx() == nullptr)
    {
        announceThreadStarted();
        return;
    }

    SocketHolder xsub(m_context, ZMQ_XSUB);
    SocketHolder xpub(m_context, ZMQ_XPUB);
    SocketHolder controlSub(m_context, ZMQ_PULL);

    const int timeoutMs = 1000;
    SocketUtil::setTimeout(xsub, timeoutMs);
    SocketUtil::setTimeout(xpub, timeoutMs);
    SocketUtil::setTimeout(controlSub, timeoutMs);

    SocketUtil::listen(xsub, m_frontendAddress);
    SocketUtil::listen(xpub, m_backendAddress);
    SocketUtil::connect(controlSub, m_controlAddress);

    announceThreadStarted();

    zmq_proxy_steerable(xsub.skt(), xpub.skt(), m_captureZMQSocket, controlSub.skt());
}

Common::ZeroMQWrapperImpl::ContextHolderSharedPtr& Common::ZeroMQWrapperImpl::ProxyImpl::ctx()
{
    return m_context;
}
