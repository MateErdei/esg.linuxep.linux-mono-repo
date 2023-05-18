// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "ISubscriber.h"

#include "modules/SubscriberLib/IEventHandler.h"
#include "modules/Heartbeat/IHeartbeat.h"

#include "Common/Threads/LockableData.h"
#include "Common/Threads/NotifyPipe.h"
#include "Common/ZMQWrapperApi/IContext.h"

#include <atomic>
#include <functional>
#include <optional>
#include <string>
#include <thread>

namespace SubscriberLib
{
    class Subscriber final : public ISubscriber
    {
    public:
        static constexpr const int DEFAULT_READ_LOOP_TIMEOUT_MS = 50000;
        Subscriber(
            std::string socketAddress,
            Common::ZMQWrapperApi::IContextSharedPtr context,
            std::unique_ptr<SubscriberLib::IEventHandler> eventQueuePusher,
            std::shared_ptr<Heartbeat::HeartbeatPinger> heartbeatPinger,
            int readLoopTimeoutMilliSeconds = DEFAULT_READ_LOOP_TIMEOUT_MS);
        ~Subscriber() override;
        void stop() noexcept final;
        void start() override;
        void restart() override;
        bool getRunningStatus() override;

    private:
        bool shouldBeRunning();
        void setIsRunning(bool);
        void setShouldBeRunning(bool);

        void subscribeToEvents();
        static std::optional<JournalerCommon::Event> convertZmqDataToEvent(Common::ZeroMQWrapper::data_t data);
        std::string m_socketPath;
        Common::Threads::LockableData<bool> m_shouldBeRunning{false};
        Common::Threads::LockableData<bool> m_isRunning{false};
        int m_readLoopTimeoutMilliSeconds;
        std::unique_ptr<std::thread> m_runnerThread;
        Common::ZMQWrapperApi::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::ISocketSubscriberPtr m_socket;
        std::unique_ptr<SubscriberLib::IEventHandler> m_eventHandler;
        std::shared_ptr<Heartbeat::HeartbeatPinger> m_heartbeatPinger;
        Common::Threads::NotifyPipe stopPipe_;
    };
} // namespace SubscriberLib
