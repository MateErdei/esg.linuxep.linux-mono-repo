/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_REACTORIMPL_H
#define EVEREST_BASE_REACTORIMPL_H

#include "IReactor.h"
#include "ICallbackListener.h"
#include "Common/Threads/AbstractThread.h"



namespace Common
{
    namespace ReactorImpl
    {

        struct ReaderListener
        {
            Common::ZeroMQWrapper::IReadable* reader;
            Reactor::ICallbackListener * listener;
        };


        class ReactorThreadImpl: public Common::Threads::AbstractThread
        {
        public:
            ReactorThreadImpl();
            ~ReactorThreadImpl();

            void addListener(Common::ZeroMQWrapper::IReadable *, Reactor::ICallbackListener *);
            void setShutdownListener(Reactor::IShutdownListener*);

        private:
            void run() override ;
            std::vector<ReaderListener> m_callbackListeners;
            Reactor::IShutdownListener *m_shutdownListener;


        };


        class ReactorImpl : public virtual Reactor::IReactor
        {
        public:
             ReactorImpl();
             void addListener(Common::ZeroMQWrapper::IReadable * readable, Reactor::ICallbackListener * callback) override;
             void armShutdownListener(Reactor::IShutdownListener * shutdownListener) override;
             void start() override;
             void stop() override;
             void join() override;
        private:
            enum ReactorState {Stopped, Started, Ready};
            std::unique_ptr<ReactorThreadImpl> m_reactorthread;
            ReactorState m_ReactorState;

        };
    }
}


#endif //EVEREST_BASE_REACTORIMPL_H
