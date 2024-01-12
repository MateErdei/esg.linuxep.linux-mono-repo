// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "PluginCallback.h"

#include "ApplicationPaths.h"
#include "Logger.h"
#include "Telemetry.h"
#include "JournalerCommon/TelemetryConsts.h"

#include "Heartbeat/ThreadIdConsts.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <utility>

#include <unistd.h>

namespace Plugin
{
    PluginCallback::PluginCallback(
        std::shared_ptr<TaskQueue> task, 
        std::shared_ptr<Heartbeat::IHeartbeat> heartbeat,
        uint64_t healthCheckDelay)
    : m_task(std::move(task))
    , m_heartbeat(heartbeat)
    , m_healthCheckDelay(healthCheckDelay)
    {
        Common::UtilityImpl::FormattedTime time;
        m_startTime = std::stoul(time.currentEpochTimeInSeconds());
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

    Health PluginCallback::getHealthInner()
    {
        Health health = Health::GOOD;
        Common::UtilityImpl::FormattedTime time;
        auto currentTime = std::stoul(time.currentEpochTimeInSeconds());
        if (currentTime < m_startTime + m_healthCheckDelay)
        {
            return Health::GOOD;
        }
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

        // Check if any pingers have not been pinged by the owning thread/worker etc.
        for (auto& kv: m_heartbeat->getMapOfIdsAgainstIsAlive())
        {
            if (!kv.second)
            {
                LOGDEBUG("A heartbeat ping was missed by: " << kv.first);
                health = Health::BAD;
            }
            telemetry.set(JournalerCommon::Telemetry::telemetryThreadHealthPrepender+kv.first, kv.second);
        }

        bool subscriberSocketExists = Common::FileSystem::fileSystem()->exists(Plugin::getSubscriberSocketPath());
        telemetry.set(JournalerCommon::Telemetry::telemetryMissingEventSubscriberSocket,
                      !subscriberSocketExists);
        if (!subscriberSocketExists)
        {
            health = Health::BAD;
        }

        bool maxAcceptableDroppedEventsExceeded = m_heartbeat->getNumDroppedEventsInLast24h() > getAcceptableDailyDroppedEvents();
        // set telemetry bool for dropped events max exceeded
        telemetry.set(JournalerCommon::Telemetry::telemetryAcceptableDroppedEventsExceeded,
                      maxAcceptableDroppedEventsExceeded);
        if (maxAcceptableDroppedEventsExceeded)
        {
            health = Health::BAD;
        }

        Common::Telemetry::TelemetryHelper::getInstance().set(JournalerCommon::Telemetry::pluginHealthStatus, static_cast<u_long>(health));
        return health;
    }

    std::string PluginCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        return "{'Health': " + std::to_string(static_cast<uint>(getHealthInner())) + "}";
    }

    uint PluginCallback::getAcceptableDailyDroppedEvents()
    {
        return 5;
    }

} // namespace Plugin