/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "QueueTask.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <EventWriterLib/IEventWriter.h>
#include <SubscriberLib/ISubscriber.h>

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
            std::unique_ptr<EventWriterLib::IEventWriter> eventWriter);
        void mainLoop();

    private:
        std::shared_ptr<QueueTask> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        std::unique_ptr<SubscriberLib::ISubscriber> m_subscriber;
        std::unique_ptr<EventWriterLib::IEventWriter> m_eventWriter;
        int QUEUE_TIMEOUT = 5; // Seconds

        void processPolicy(const std::string& policyXml);
    };
} // namespace Plugin
