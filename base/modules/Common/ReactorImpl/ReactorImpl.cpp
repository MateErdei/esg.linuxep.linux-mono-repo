/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <cassert>
#include <signal.h>
#include <memory>
#include <iostream>
#include "ReactorImpl.h"
#include "IPoller.h"

#define LOGERROR(x) std::cerr << x << '\n'
#define LOGDEBUG(x) std::cerr << x << '\n'

namespace
{
    static std::unique_ptr<Common::Threads::NotifyPipe> signalPipe;

    static void s_signal_handler (int signal_value)
    {
        LOGDEBUG( "Signal received: " << signal_value );
        if ( !signalPipe)
        {
            return;
        }
        signalPipe->notify();
    }

}

namespace Common
{
    namespace ReactorImpl
    {
        using namespace Common::Reactor;
        ReactorThreadImpl::ReactorThreadImpl()
                : m_shutdownListener(nullptr)
        {

        }

        ReactorThreadImpl::~ReactorThreadImpl()
        {
            m_callbackListeners.clear();
            if ( signalPipe )
            {
                signalPipe.reset(nullptr);
            }

        }

        void ReactorThreadImpl::addListener(Common::ZeroMQWrapper::IReadable * readable, ICallbackListener * callback)
        {
            ReaderListener entry;
            entry.reader = readable;
            entry.listener = callback;
            //LOGDEBUG("add listeners: " << entry.reader->fd());
            m_callbackListeners.push_back(entry);
        };


        void ReactorThreadImpl::setShutdownListener(IShutdownListener* shutdownListener )
        {
            assert(shutdownListener!= nullptr);

            m_shutdownListener =  shutdownListener;
        }

        void ReactorThreadImpl::run()
        {
            Common::ZeroMQWrapper::IHasFDPtr shutdownPipePtr;
            //LOGERROR("Running Thread Impl");
            announceThreadStarted();


            Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

            for ( auto & readList : m_callbackListeners)
            {
                //LOGDEBUG("add call back: " << readList.reader->fd());
                poller->addEntry(*(readList.reader), Common::ZeroMQWrapper::IPoller::POLLIN);
            }

            if ( m_shutdownListener )
            {

                // add the shutdown listener
                signalPipe = std::unique_ptr<Common::Threads::NotifyPipe>( new Common::Threads::NotifyPipe());
                struct sigaction action;
                action.sa_handler = s_signal_handler;
                action.sa_flags = 0;
                sigemptyset (&action.sa_mask);
                sigaction (SIGINT, &action, NULL);
                sigaction (SIGTERM, &action, NULL);

                shutdownPipePtr = poller->addEntry(signalPipe.get()->readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

            }

            //LOGDEBUG("add pipe call back");
            auto stopRequestedFD = poller->addEntry(m_notifyPipe.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

            bool callBackRequestedStop = false;
            while ( !stopRequested() && !callBackRequestedStop)
            {
                auto filedescriptors = poller->poll(Common::ZeroMQWrapper::ms(-1));

                //LOGDEBUG("activity in the file descriptor: " << hasFd->fd());
                if ( m_shutdownListener &&  signalPipe && signalPipe->notified())
                {

                    m_shutdownListener->notifyShutdownRequested();
                    return;
                }

                for( auto & hasFd : filedescriptors )
                {

                    for( auto & ireader : m_callbackListeners)
                    {
                        //LOGDEBUG("check callbacklisteners: " << ireader.reader->fd());
                        if ( hasFd->fd() == ireader.reader->fd())
                        {
                            Common::ZeroMQWrapper::IReadable::data_t request = ireader.reader->read();
                            try
                            {
                                ireader.listener->messageHandler(request);
                            }
                            catch ( StopReactorRequestException & )
                            {
                                callBackRequestedStop = true;
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
            assert(m_ReactorState == ReactorState::Ready);
            m_reactorthread->addListener(readable, callback);
        }

        void ReactorImpl::armShutdownListener(IShutdownListener * shutdownListener)
        {
            assert(m_ReactorState == ReactorState::Ready);
            m_reactorthread->setShutdownListener(shutdownListener);
        }

        void ReactorImpl::start()
        {
            assert(m_ReactorState == ReactorState::Ready);
            m_ReactorState = ReactorState::Started;
            m_reactorthread->start();
        }

        void ReactorImpl::stop()
        {
            if ( m_ReactorState == ReactorState::Stopped)
            {
                return;
            }

            m_ReactorState = ReactorState::Stopped;
            m_reactorthread->requestStop();
            // wait for the thread to stop
            m_reactorthread.reset();
        }

        ReactorImpl::ReactorImpl()
        {
            m_ReactorState = ReactorState::Ready;
            m_reactorthread = std::unique_ptr<ReactorThreadImpl>(new ReactorThreadImpl());
        }

        void ReactorImpl::join()
        {
            m_ReactorState = ReactorState::Ready;
            m_reactorthread->join();
        }
    }
}
std::unique_ptr<Common::Reactor::IReactor>  Common::Reactor::createReactor()
{
    return Common::Reactor::IReactorPtr(new Common::ReactorImpl::ReactorImpl());
}
