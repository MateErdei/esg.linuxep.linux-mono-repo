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
    namespace Reactor
    {

        struct ReaderListener
        {
            Common::ZeroMQWrapper::IReadable* reader;
            ICallbackListener * listener;
        };


        class ReactorThreadImpl: public Common::Threads::AbstractThread
        {
        public:
            ReactorThreadImpl();
            ~ReactorThreadImpl();

            void addListener(Common::ZeroMQWrapper::IReadable *, ICallbackListener *);
            void setShutdownListener(IShutdownListener*);

        private:
            void run() override ;
            std::vector<ReaderListener> m_callbacListeners;
            IShutdownListener *m_shutdownListener;


        };


        class ReactorImpl : public virtual IReactor
        {
        public:
            enum ReactorSate {Stopped, Started};
             ReactorImpl();
             void addListener(Common::ZeroMQWrapper::IReadable *, ICallbackListener *) override;
             void armShutdownListener(IShutdownListener *) override;
             void start() override;
             void stop() override;
             void join() override;
        private:
            std::unique_ptr<ReactorThreadImpl> m_reactorthread;
        };
    }
}


#endif //EVEREST_BASE_REACTORIMPL_H
