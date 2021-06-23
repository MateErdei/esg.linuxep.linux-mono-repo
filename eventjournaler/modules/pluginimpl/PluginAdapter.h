/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "QueueTask.h"
#include <SubscriberLib/ISubscriber.h>

#include <Common/PluginApi/IBaseServiceApi.h>

namespace Plugin
{
    class DetectRequestToStop : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class PluginAdapter
    {
    public:
        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            std::unique_ptr<SubscriberLib::ISubscriber> subscriber);
        void mainLoop();

        static std::string
        waitForTheFirstPolicy(QueueTask& queueTask, std::chrono::seconds timeoutInS, int maxTasksThreshold,
                              const std::string& policyAppId);

    private:
        std::shared_ptr<QueueTask> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        std::unique_ptr<SubscriberLib::ISubscriber> m_subscriber;
        int QUEUE_TIMEOUT = 5; // Seconds

        void processPolicy(const std::string& policyXml);
    };
} // namespace Plugin
