// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "config.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/ErrorCodes.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <EventJournal/EventJournalWriter.h>
#include <EventWriterWorkerLib/EventQueuePopper.h>
#include <EventWriterWorkerLib/EventWriterWorker.h>
#include <modules/SubscriberLib/EventQueuePusher.h>
#include <modules/SubscriberLib/Subscriber.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <modules/pluginimpl/Logger.h>
#include <modules/pluginimpl/PluginAdapter.h>
#include <modules/Heartbeat/Heartbeat.h>
#include <modules/Heartbeat/ThreadIdConsts.h>

// Auto version headers
#include "products/distribution/include/AutoVersioningHeaders/AutoVersion.h"

const char* PluginName = PLUGIN_NAME;
const int MAX_QUEUE_SIZE = 100;

int main()
{
    using namespace Plugin;
    int ret = 0;
    Common::Logging::PluginLoggingSetup loggerSetup(PluginName);
    LOGINFO("Event Journaler " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");

    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
        Common::PluginApi::createPluginResourceManagement();

    auto queueTask = std::make_shared<QueueTask>();
    auto heartbeat = std::make_shared<Heartbeat::Heartbeat>();
    auto sharedPluginCallBack = std::make_shared<PluginCallback>(queueTask, heartbeat);

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

    std::shared_ptr<EventQueueLib::EventQueue> eventQueue(new EventQueueLib::EventQueue(MAX_QUEUE_SIZE));

    std::unique_ptr<SubscriberLib::IEventHandler> eventQueuePusher(new SubscriberLib::EventQueuePusher(eventQueue));

    std::unique_ptr<SubscriberLib::ISubscriber> subscriber(
            new SubscriberLib::Subscriber(
                    Plugin::getSubscriberSocketPath(),
                    Common::ZMQWrapperApi::createContext(),
                    std::move(eventQueuePusher),
                    heartbeat->getPingHandleForId(Heartbeat::getSubscriberThreadId())));

    std::unique_ptr<EventJournal::IEventJournalWriter> eventJournalWriter (new EventJournal::Writer());

    std::shared_ptr<EventWriterLib::IEventWriterWorker> eventWriter(
            new EventWriterLib::EventWriterWorker(
        eventQueue,
        std::move(eventJournalWriter),
        heartbeat->getPingHandleForId(Heartbeat::getWriterThreadId())));

    PluginAdapter pluginAdapter(
            queueTask,
            std::move(baseService),
            sharedPluginCallBack,
            std::move(subscriber),
            eventWriter,
            heartbeat->getPingHandleForId(Heartbeat::getPluginAdapterThreadId()));

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
