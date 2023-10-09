// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ReactorImpl.h"

#include "modules/Common/ZeroMQWrapper/IPoller.h"
#include "Logger.h"

#include "modules/Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h"
#include "modules/Common/ZeroMQWrapperImpl/PollerFactory.h"

#include <cassert>
#include <csignal>
#include <memory>

namespace
{
    std::mutex GL_signalMutex;
    std::unique_ptr<Common::Threads::NotifyPipe> GL_signalPipe;

    void s_signal_handler(int)
    {
        if (!GL_signalPipe)
        {
            return;
        }
        GL_signalPipe->notify();
    }

} // namespace

namespace Common
{
    namespace ReactorImpl
    {
        using namespace Common::Reactor;
        ReactorThreadImpl::ReactorThreadImpl() : m_shutdownListener(nullptr) {}

        ReactorThreadImpl::~ReactorThreadImpl() { m_callbackListeners.clear(); }

        void ReactorThreadImpl::addListener(Common::ZeroMQWrapper::IReadable* readable, ICallbackListener* callback)
        {
            ReaderListener entry{ .reader = readable, .listener = callback };
            // LOGDEBUG("add listeners: " << entry.reader->fd());
            m_callbackListeners.push_back(entry);
        }

        void ReactorThreadImpl::setShutdownListener(IShutdownListener* shutdownListener)
        {
            assert(shutdownListener != nullptr);

            m_shutdownListener = shutdownListener;
        }

        void ReactorThreadImpl::main_loop()
        {
            Common::ZeroMQWrapper::IHasFDPtr shutdownPipePtr;
            // LOGERROR("Running Thread Impl");
            bool monitorForSignalsForShutdown = false;

            Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::pollerFactory().create();

            for (auto& readList : m_callbackListeners)
            {
                // LOGDEBUG("add call back: " << readList.reader->fd());
                poller->addEntry(*(readList.reader), Common::ZeroMQWrapper::IPoller::POLLIN);
            }

            if (m_shutdownListener)
            {
                std::lock_guard<std::mutex> mutexLock(GL_signalMutex);
                if (GL_signalPipe != nullptr)
                {
                    LOGERROR("Attempting to configure shutdown monitor in ReactorThread while another ReactorThread is "
                             "running");
                }
                else
                {
                    // add the onShutdown listener
                    GL_signalPipe = std::make_unique<Common::Threads::NotifyPipe>();
                    struct sigaction action{};
                    action.sa_handler = s_signal_handler;
                    action.sa_flags = SA_RESTART;
                    sigemptyset(&action.sa_mask);
                    sigaction(SIGINT, &action, nullptr);
                    sigaction(SIGTERM, &action, nullptr);

                    shutdownPipePtr = poller->addEntry(GL_signalPipe->readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);
                    monitorForSignalsForShutdown = true;
                }
            }

            // LOGDEBUG("add pipe call back");
            auto stopRequestedFD = poller->addEntry(m_notifyPipe.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);
            announceThreadStarted();
            bool callBackRequestedStop = false;
            while (!stopRequested() && !callBackRequestedStop)
            {
                auto filedescriptors = poller->poll(Common::ZeroMQWrapper::ms(100000));

                // LOGDEBUG("activity in the file descriptor: " << hasFd->fd());
                if (monitorForSignalsForShutdown && GL_signalPipe->notified())
                {
                    m_shutdownListener->notifyShutdownRequested();
                    break;
                }

                for (auto& hasFd : filedescriptors)
                {
                    for (auto& ireader : m_callbackListeners)
                    {
                        // LOGDEBUG("check callbacklisteners: " << ireader.reader->fd());
                        if (hasFd->fd() == ireader.reader->fd())
                        {
                            Common::ZeroMQWrapper::IReadable::data_t request;
                            try
                            {
                                request = ireader.reader->read();
                            }
                            catch (std::exception& ex)
                            {
                                LOGERROR("Reactor: failed to read message from callback listener: " << ex.what());
                                throw;
                            }
                            try
                            {
                                ireader.listener->messageHandler(request);
                            }
                            catch (StopReactorRequestException&)
                            {
                                callBackRequestedStop = true;
                            }
                            catch (std::exception& ex)
                            {
                                LOGERROR("Reactor: callback process failed with message: " << ex.what());
                            }
                        }
                    }
                }
            }

            // Clear signal handlers
            if (monitorForSignalsForShutdown)
            {
                // Reset signal handlers if we set them before
                struct sigaction action{};
                action.sa_handler = SIG_DFL;
                action.sa_flags = SA_RESTART;
                sigemptyset(&action.sa_mask);
                sigaction(SIGINT, &action, nullptr);
                sigaction(SIGTERM, &action, nullptr);

                GL_signalPipe.reset();
            }
        }

        void ReactorThreadImpl::run()
        {
            try
            {
                main_loop();
                // since the reactor run in a different thread, the safest option when the system fails is to terminate.
            }
            catch (Common::ZeroMQWrapperImpl::ZeroMQPollerException& ex)
            {
                LOGERROR("Error associated with the poller: " << ex.what());
                std::terminate();
            }
            catch (std::exception& ex)
            {
                LOGERROR(ex.what());
                std::terminate();
            }
        }

        void ReactorImpl::addListener(Common::ZeroMQWrapper::IReadable* readable, ICallbackListener* callback)
        {
            assert(m_ReactorState == ReactorState::Ready);
            m_reactorthread->addListener(readable, callback);
        }

        void ReactorImpl::armShutdownListener(IShutdownListener* shutdownListener)
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
            if (m_ReactorState == ReactorState::Stopped)
            {
                return;
            }

            m_ReactorState = ReactorState::Stopped;
            m_reactorthread->requestStop();
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

        ReactorImpl::~ReactorImpl()
        {
            // cppcheck-suppress virtualCallInConstructor
            stop();
            // cppcheck-suppress virtualCallInConstructor
            join();
        }
    } // namespace ReactorImpl
} // namespace Common
std::unique_ptr<Common::Reactor::IReactor> Common::Reactor::createReactor()
{
    return Common::Reactor::IReactorPtr(new Common::ReactorImpl::ReactorImpl());
}
