/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Reactor/ICallbackListener.h"
#include "Common/Reactor/IReactor.h"
#include "Common/Threads/AbstractThread.h"

namespace Common
{
    namespace ReactorImpl
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
            ~ReactorThreadImpl();

            void addListener(Common::ZeroMQWrapper::IReadable*, Reactor::ICallbackListener*);
            void setShutdownListener(Reactor::IShutdownListener*);

        private:
            void run() override;
            void main_loop();
            std::vector<ReaderListener> m_callbackListeners;
            Reactor::IShutdownListener* m_shutdownListener;
        };

        class ReactorImpl : public virtual Reactor::IReactor
        {
        public:
            ReactorImpl();

            ~ReactorImpl();
            void addListener(Common::ZeroMQWrapper::IReadable* readable, Reactor::ICallbackListener* callback) override;
            void armShutdownListener(Reactor::IShutdownListener* shutdownListener) override;
            void start() override;
            void stop() override;
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
    } // namespace ReactorImpl
} // namespace Common
