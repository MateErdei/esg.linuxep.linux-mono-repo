// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISubscriber.h"

#include "IEventHandler.h"

#include "Heartbeat/IHeartbeat.h"
#include "JournalerCommon/TimeConsts.h"

#include "Common/Threads/LockableData.h"
#include "Common/Threads/NotifyPipe.h"
#include "Common/ZMQWrapperApi/IContext.h"

#include <atomic>
#include <functional>
#include <optional>
#include <string>
#include <thread>

#ifndef TEST_PUBLIC
#define TEST_PUBLIC private
#endif

namespace SubscriberLib
{
    class Subscriber final : public ISubscriber
    {
    public:
        Subscriber(
            std::string socketAddress,
            Common::ZMQWrapperApi::IContextSharedPtr context,
            std::unique_ptr<SubscriberLib::IEventHandler> eventQueuePusher,
            std::shared_ptr<Heartbeat::HeartbeatPinger> heartbeatPinger,
            int readLoopTimeoutMilliSeconds = JournalerCommon::DEFAULT_READ_LOOP_TIMEOUT_MS);
        ~Subscriber() override;
        void stop() noexcept final;
        void start() override;
        void restart() override;
        bool getRunningStatus() override;

    TEST_PUBLIC:
        bool getSubscriberFinished();

    private:
        bool shouldBeRunning();
        void setIsRunning(bool) noexcept;
        void setShouldBeRunning(bool);
        void setSubscriberFinished(bool) noexcept;

        /**
         * Set subscriberFinished_ to true and m_isRunning to false
         */
        void subscriberFinished() noexcept;

        void subscribeToEvents();
        static std::optional<JournalerCommon::Event> convertZmqDataToEvent(Common::ZeroMQWrapper::data_t data);
        std::string m_socketPath;
        Common::Threads::LockableData<bool> m_shouldBeRunning{false};
        Common::Threads::LockableData<bool> m_isRunning{false};
        Common::Threads::LockableData<bool> subscriberFinished_{false};
        int m_readLoopTimeoutMilliSeconds;
        std::unique_ptr<std::thread> m_runnerThread;
        Common::ZMQWrapperApi::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::ISocketSubscriberPtr m_socket;
        std::unique_ptr<SubscriberLib::IEventHandler> m_eventHandler;
        std::shared_ptr<Heartbeat::HeartbeatPinger> m_heartbeatPinger;
        Common::Threads::NotifyPipe stopPipe_;
    };
} // namespace SubscriberLib
