/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <cassert>
#include <signal.h>
#include <memory>
#include <iostream>
#include "ReactorImpl.h"
#include "IPoller.h"
#include "Common/Threads/NotifyPipe.h"

#define LOGERROR(x) std::cerr << x << '\n'

namespace
{
    static std::unique_ptr<Common::Threads::NotifyPipe> signalPipe;

    static void s_signal_handler (int /*signal_value*/)
    {
        if ( !signalPipe)
        {
            return;
        }
        signalPipe->notify();
    }

}

namespace Common
{
    namespace Reactor
    {

        ReactorThreadImpl::ReactorThreadImpl() :
                m_shutdownListener(nullptr)
        {

        }

        ReactorThreadImpl::~ReactorThreadImpl()
        {
            if ( signalPipe )
            {
                signalPipe.reset(nullptr);
            }
        }


        void ReactorThreadImpl::addListener(Common::ZeroMQWrapper::IReadable * readable, ICallbackListener * callback)
        {
            ReaderListener entry{readable, callback};
            m_callbacListeners.push_back(entry);
        };


        void ReactorThreadImpl::setShutdownListener(IShutdownListener* shutdownListener )
        {
            assert(shutdownListener!= nullptr);

            m_shutdownListener =  shutdownListener;
        }

        void ReactorThreadImpl::run()
        {
            announceThreadStarted();

            Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

            for ( auto & readList : m_callbacListeners)
            {
                poller->addEntry(*readList.reader, Common::ZeroMQWrapper::IPoller::POLLIN);
            }

            if ( m_shutdownListener )
            {
                // add the shutdown listener
                signalPipe = std::unique_ptr<Common::Threads::NotifyPipe>();
                struct sigaction action;
                action.sa_handler = s_signal_handler;
                action.sa_flags = 0;
                sigemptyset (&action.sa_mask);
                sigaction (SIGINT, &action, NULL);
                sigaction (SIGTERM, &action, NULL);
            }

            auto stopRequestedFD = poller->addEntry(m_notifyPipe.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);


            while ( !stopRequested())
            {
                auto filedescriptors = poller->poll(Common::ZeroMQWrapper::ms(-1));

                for( auto & hasFd : filedescriptors )
                {

                    if ( m_shutdownListener &&  signalPipe && signalPipe->notified())
                    {
                        m_shutdownListener->notifyShutdownRequested();
                    }


                    for( auto & ireader : m_callbacListeners)
                    {
                        if ( hasFd->fd() == ireader.reader->fd())
                        {
                            std::vector<std::string> request = ireader.reader->read();
                            try
                            {
                                ireader.listener->process(request);
                            }
                            catch ( std::exception & ex)
                            {
                                LOGERROR("Reactor: callback process failed with message: " << ex.what());
                            }
                        }

                    }
                }

            }
        }

        void ReactorImpl::addListener(Common::ZeroMQWrapper::IReadable * readable, ICallbackListener * callback)
        {
            m_reactorthread->addListener(readable, callback);
        }

        void ReactorImpl::armShutdownListener(IShutdownListener * shutdownListener)
        {
            m_reactorthread->setShutdownListener(shutdownListener);
        }

        void ReactorImpl::start()
        {
            m_reactorthread->start();
        }

        void ReactorImpl::stop()
        {
            m_reactorthread->requestStop();
        }
    }
}
