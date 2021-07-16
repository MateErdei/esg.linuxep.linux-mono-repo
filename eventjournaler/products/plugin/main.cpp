/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "config.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/ErrorCodes.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <modules/SubscriberLib/Subscriber.h>
#include <modules/SubscriberLib/EventQueuePusher.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <modules/pluginimpl/Logger.h>
#include <modules/pluginimpl/PluginAdapter.h>

const char* PluginName = PLUGIN_NAME;

int main()
{
    using namespace Plugin;
    int ret = 0;
    Common::Logging::PluginLoggingSetup loggerSetup(PluginName);

    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
        Common::PluginApi::createPluginResourceManagement();

    auto queueTask = std::make_shared<QueueTask>();
    auto sharedPluginCallBack = std::make_shared<PluginCallback>(queueTask);

    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;

    try
    {
        baseService = resourceManagement->createPluginAPI(PluginName, sharedPluginCallBack);
    }
    catch (const Common::PluginApi::ApiException& apiException)
    {
        LOGERROR("Plugin Api could not be instantiated: " << apiException.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }

    auto context = Common::ZMQWrapperApi::createContext();

    EventQueueLib::EventQueue* eventQueue = new EventQueueLib::EventQueue(100);
    std::shared_ptr<EventQueueLib::EventQueue> eventQueuePtr(eventQueue);
    SubscriberLib::IEventQueuePusher* pusher = new SubscriberLib::EventQueuePusher(eventQueuePtr);
    std::unique_ptr<SubscriberLib::IEventQueuePusher> pusherPtr(pusher);

    std::unique_ptr<SubscriberLib::ISubscriber> subscriber =
        std::make_unique<SubscriberLib::Subscriber>(Plugin::getSubscriberSocketPath(), context, std::move(pusherPtr));
    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack, std::move(subscriber));

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Plugin threw an exception at top level: " << ex.what());
        ret = 40;
    }
    sharedPluginCallBack->setRunning(false);
    LOGINFO("Plugin Finished.");
    return ret;
}
