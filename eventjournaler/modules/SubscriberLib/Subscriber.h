/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ISubscriber.h"

#include <Common/ZMQWrapperApi/IContext.h>
#include <modules/SubscriberLib/IEventQueuePusher.h>

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace SubscriberLib
{
    class Subscriber : public ISubscriber
    {
    public:
        Subscriber(
            const std::string& socketAddress,
            Common::ZMQWrapperApi::IContextSharedPtr context,
            std::unique_ptr<SubscriberLib::IEventQueuePusher> eventQueuePusher,
            int readLoopTimeoutMilliSeconds = 5000);
        ~Subscriber() override;
        void stop() override;
        void start() override;
        void restart() override;
        bool getRunningStatus() override;

    private:
        void subscribeToEvents();
        std::string m_socketPath;
        std::atomic<bool> m_running = false;
        int m_readLoopTimeoutMilliSeconds;
        std::unique_ptr<std::thread> m_runnerThread;
        Common::ZMQWrapperApi::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::ISocketSubscriberPtr m_socket;
        std::unique_ptr<SubscriberLib::IEventQueuePusher> m_eventQueuePusher;
    };
} // namespace SubscriberLib
