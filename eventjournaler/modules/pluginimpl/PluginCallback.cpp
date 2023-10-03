// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "PluginCallback.h"

#include "ApplicationPaths.h"
#include "Logger.h"
#include "Telemetry.h"
#include "JournalerCommon/TelemetryConsts.h"

#include "Heartbeat/ThreadIdConsts.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <utility>

#include <unistd.h>

namespace Plugin
{
    PluginCallback::PluginCallback(std::shared_ptr<TaskQueue> task, std::shared_ptr<Heartbeat::IHeartbeat> heartbeat) :
    m_task(std::move(task)),
    m_heartbeat(heartbeat)
    {
        LOGDEBUG("Plugin Callback Started");
        std::vector<std::string> ids{Heartbeat::WriterThreadId, Heartbeat::SubscriberThreadId, Heartbeat::PluginAdapterThreadId};
        m_heartbeat->registerIds(ids);
    }

    void PluginCallback::applyNewPolicy(const std::string& /*policyXml */)
    {
        LOGSUPPORT("Received unexpected policy");
    }

    void PluginCallback::queueAction(const std::string& /* actionXml */) { LOGSUPPORT("Received unexpected action"); }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_task->pushStop();
        int timeoutCounter = 0;
        int shutdownTimeout = 30;
        while(isRunning() && timeoutCounter < shutdownTimeout)
        {
            LOGSUPPORT("Shutdown waiting for all processes to complete");
            sleep(1);
            timeoutCounter++;
        }
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string& /* appId */)
    {
        LOGSUPPORT("Received unexpected get status request");
        return Common::PluginApi::StatusInfo{};
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

        getHealthInner();

        std::optional<std::string> version = Plugin::getVersion();
        if (version)
        {
            telemetry.set(JournalerCommon::Telemetry::version, version.value());
        }
        std::string telemetryJson = telemetry.serialiseAndReset();
        LOGDEBUG("Got telemetry JSON data: " << telemetryJson);

        return telemetryJson;
    }

    void PluginCallback::setRunning(bool running)
    {
        m_running = running;
    }

    bool PluginCallback::isRunning()
    {
        return m_running;
    }

    uint PluginCallback::getHealthInner()
    {
        uint health = 0;
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

        // Check if any pingers have not been pinged by the owning thread/worker etc.
        for (auto& kv: m_heartbeat->getMapOfIdsAgainstIsAlive())
        {
            if (!kv.second)
            {
                LOGDEBUG("A heartbeat ping was missed by: " << kv.first);
                health = 1;
            }
            telemetry.set(JournalerCommon::Telemetry::telemetryThreadHealthPrepender+kv.first, kv.second);
        }

        bool subscriberSocketExists = Common::FileSystem::fileSystem()->exists(Plugin::getSubscriberSocketPath());
        telemetry.set(JournalerCommon::Telemetry::telemetryMissingEventSubscriberSocket,
                      !subscriberSocketExists);
        if (!subscriberSocketExists)
        {
            health = 1;
        }

        bool maxAcceptableDroppedEventsExceeded = m_heartbeat->getNumDroppedEventsInLast24h() > getAcceptableDailyDroppedEvents();
        // set telemetry bool for dropped events max exceeded
        telemetry.set(JournalerCommon::Telemetry::telemetryAcceptableDroppedEventsExceeded,
                      maxAcceptableDroppedEventsExceeded);
        if (maxAcceptableDroppedEventsExceeded)
        {
            health = 1;
        }

        Common::Telemetry::TelemetryHelper::getInstance().set(JournalerCommon::Telemetry::pluginHealthStatus, static_cast<u_long>(health));
        return health;
    }

    std::string PluginCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        return "{'Health': " + std::to_string(getHealthInner()) + "}";
    }

    uint PluginCallback::getAcceptableDailyDroppedEvents()
    {
        return 5;
    }

} // namespace Plugin