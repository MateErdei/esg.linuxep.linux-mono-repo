/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/
//#include <Subscriber/Subscriber.h>
#include "PluginAdapter.h"

#include "Logger.h"
#include "Telemetry.h"
#include "ApplicationPaths.h"
#include <Common/PluginApi/NoPolicyAvailableException.h>
//#include <modules/SubscriberLib/Subscriber.h>
#include <SubscriberLib/Subscriber.h>

#include <unistd.h>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback))
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        LOGINFO("Entering the main loop");
        SubscriberLib::Subscriber subscriber(Plugin::getSubscriberSocketPath());
        subscriber.start();
        sleep(11);
        subscriber.stop();
        std::cout << "DONE";
//        subscriber.subscribeToEvents();

        // If the plugin requires a mcs policy the plugin needs to explicitly request the policy on start-up
        // or the plugin will not receive the policy until the next time the policy changes.
        // the try catch code below uses the ALC policy as an example.
//        try
//        {
//            // Request required policies for plugin.
//            m_baseService->requestPolicies("ALC");
//        }
//        catch (const Common::PluginApi::NoPolicyAvailableException&)
//        {
//            LOGINFO("No policy available right now for app: " << "ALC");
//            // Ignore no Policy Available errors
//        }



//        while (true)
//        {
//            Task task = m_queueTask->pop();
//            switch (task.taskType)
//            {
//                case Task::TaskType::Stop:
//                    return;
//
//                case Task::TaskType::Policy:
//                    processPolicy(task.Content);
//                    break;
//            }
//        }
    }

    void PluginAdapter::processPolicy(const std::string& policyXml) { LOGDEBUG("Process policy: " << policyXml); }
} // namespace Plugin