/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Reactor/ICallbackListener.h"
#include "Common/Reactor/IReactor.h"
#include "Common/Threads/AbstractThread.h"

namespace Common::ReactorImpl
{
    struct ReaderListener
    {
        Common::ZeroMQWrapper::IReadable* reader;
        Reactor::ICallbackListener* listener;
    };

    class ReactorThreadImpl : public Common::Threads::AbstractThread
    {
    public:
        ReactorThreadImpl();
        ~ReactorThreadImpl() override;

        void addListener(Common::ZeroMQWrapper::IReadable*, Reactor::ICallbackListener*);
        void setShutdownListener(Reactor::IShutdownListener*);

    private:
        void run() override;
        void main_loop();
        std::vector<ReaderListener> m_callbackListeners;
        Reactor::IShutdownListener* m_shutdownListener;
    };

    class ReactorImpl final : public virtual Reactor::IReactor
    {
    public:
        ReactorImpl();

        ~ReactorImpl() override;
        void addListener(Common::ZeroMQWrapper::IReadable* readable, Reactor::ICallbackListener* callback) override;
        void armShutdownListener(Reactor::IShutdownListener* shutdownListener) override;
        void start() override;
        // cppcheck-suppress virtualCallInConstructor
        void stop() override;
        // cppcheck-suppress virtualCallInConstructor
        void join() override;

    private:
        enum ReactorState
        {
            Stopped,
            Started,
            Ready
        };
        std::unique_ptr<ReactorThreadImpl> m_reactorthread;
        ReactorState m_ReactorState;
    };
} // namespace Common::ReactorImpl
