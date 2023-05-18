// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "ISubscriber.h"

#include "modules/SubscriberLib/IEventHandler.h"
#include "modules/Heartbeat/IHeartbeat.h"

#include "Common/Threads/LockableData.h"
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
        Subscriber(
            std::string socketAddress,
            Common::ZMQWrapperApi::IContextSharedPtr context,
            std::unique_ptr<SubscriberLib::IEventHandler> eventQueuePusher,
            std::shared_ptr<Heartbeat::HeartbeatPinger> heartbeatPinger,
            int readLoopTimeoutMilliSeconds = 5000);
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
    };
} // namespace SubscriberLib
