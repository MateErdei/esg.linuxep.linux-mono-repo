/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "QueueTask.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <EventWriterWorkerLib/IEventWriterWorker.h>
#include <SubscriberLib/ISubscriber.h>
#include <modules/Heartbeat/HeartbeatPinger.h>

namespace Plugin
{
    class PluginAdapter
    {
    public:
        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            std::unique_ptr<SubscriberLib::ISubscriber> subscriber,
            std::shared_ptr<EventWriterLib::IEventWriterWorker> eventWriter,
            Heartbeat::HeartbeatPinger heartbeatPinger);
        void mainLoop();

    private:
        std::shared_ptr<QueueTask> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        std::unique_ptr<SubscriberLib::ISubscriber> m_subscriber;
        std::shared_ptr<EventWriterLib::IEventWriterWorker> m_eventWriterWorker;
        Heartbeat::HeartbeatPinger m_heartbeatPinger;

        int QUEUE_TIMEOUT = 5; // Seconds

        void processPolicy(const std::string& policyXml);
        void manageJournalDataStoreSize();

        uint64_t dataLimit = 3600000000; //3.6GB
        uint64_t lowerLimit= 3000000000; //3.0GB
    };
} // namespace Plugin
