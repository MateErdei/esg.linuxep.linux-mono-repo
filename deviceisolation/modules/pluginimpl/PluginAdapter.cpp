// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginAdapter.h"

#include "ActionProcessor.h"
#include "ApplicationPaths.h"
#include "Logger.h"
#include "NftWrapper.h"
#include "NTPPolicy.h"
#include "NTPStatus.h"
#include "TelemetryConsts.h"

#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/PluginApi/NoPolicyAvailableException.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <utility>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
            std::shared_ptr<TaskQueue> queueTask,
            std::shared_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback)
            : taskQueue_(std::move(queueTask))
            , baseService_(std::move(baseService))
            , callback_(std::move(callback))
            , ntpPolicy_(std::make_shared<NTPPolicy>())
    {
    }

    void PluginAdapter::mainLoop()
    {
        callback_->setRunning(true);
        using clock_t = std::chrono::steady_clock;
        auto start = clock_t::now();
        auto queue_timeout = queueTimeout_;
        bool logMissingPolicyWarning = false;
        bool logMissingPolicyDebug = false;

        try
        {
            baseService_->requestPolicies("NTP");
        }
        catch (const Common::PluginApi::NoPolicyAvailableException&)
        {
            LOGINFO("NTP policy not available immediately");
            // Ignore no Policy Available errors
        }

        Common::Telemetry::TelemetryHelper::getInstance().set(
                DeviceIsolation::Telemetry::activatedInPast24HoursKey, false);

        Common::Telemetry::TelemetryObject isolationEnabledTelemetry;
        isolationEnabledTelemetry.set(Common::Telemetry::TelemetryValue(isolationEnabled_));
        Common::Telemetry::TelemetryHelper::getInstance().set(
                DeviceIsolation::Telemetry::currentlyActiveKey, isolationEnabledTelemetry, true);

        LOGINFO("Completed initialization of Device Isolation");
        while (true)
        {
            Task task;
            if (!taskQueue_->pop(task, queue_timeout))
            {
                LOGDEBUG("Timed out waiting for task");
                if (ntpPolicy_->revId().empty())
                {
                    auto duration = clock_t::now() - start;
                    if (duration > warnMissingPolicyTimeout_)
                    {
                        if (!logMissingPolicyWarning)
                        {
                            LOGWARN("Failed to get NTP policy within "
                                << std::chrono::duration_cast<std::chrono::seconds>(warnMissingPolicyTimeout_).count()
                                << " seconds");
                            logMissingPolicyWarning = true;
                            queue_timeout = MAXIMUM_TIMEOUT;
                            sendStatus();
                        }
                    }
                    else if (duration > debugMissingPolicyTimeout_)
                    {
                        if (!logMissingPolicyDebug)
                        {
                            LOGDEBUG("Failed to get NTP policy within "
                                << std::chrono::duration_cast<std::chrono::seconds>(debugMissingPolicyTimeout_).count()
                                << " seconds");
                            logMissingPolicyDebug = true;
                            queue_timeout = warnMissingPolicyTimeout_;
                        }
                    }
                }
                else
                {
                    LOGDEBUG("Timeout when we already have policy: " << ntpPolicy_->revId());
                    queue_timeout = MAXIMUM_TIMEOUT;
                }
            }
            else
            {
                switch (task.taskType)
                {
                    case Task::TaskType::Stop:
                        return;
                    case Task::TaskType::Policy:
                        processPolicy(task.appId, task.Content);
                        break;
                    case Task::TaskType::Action:
                        processAction(task.Content);
                        break;
                }
                sendStatus();
            }
        }
    }

    void PluginAdapter::processAction(const std::string& actionXml)
    {
        LOGDEBUG("Received action: " << actionXml);
        auto action = ActionProcessor::processIsolateAction(actionXml);
        if (!action)
        {
            // Error logged in processIsolateAction
            LOGDEBUG("Ignoring invalid action: " << actionXml);
            return;
        }

        if (action.value())
        {
            LOGINFO("Enabling Device Isolation");
            auto result = NftWrapper::applyIsolateRules(ntpPolicy_->exclusions());
            if (result != NftWrapper::IsolateResult::SUCCESS)
            {
                LOGERROR("Failed to isolate device");
                return;
            }
            isolationEnabled_ = true;
            Common::Telemetry::TelemetryHelper::getInstance().set(
                    DeviceIsolation::Telemetry::activatedInPast24HoursKey, true);
            LOGINFO("Device is now isolated");
        }
        else
        {
            LOGINFO("Disabling Device Isolation");
            auto result = NftWrapper::clearIsolateRules();
            if (result == NftWrapper::IsolateResult::FAILED)
            {
                LOGERROR("Failed to remove device from isolation");
                return;
            }
            else if (result == NftWrapper::IsolateResult::RULES_NOT_PRESENT)
            {
                if (isolationEnabled_)
                {
                    LOGERROR("Failed to list sophos rules table but isolation is enabled");
                    return;
                }
                LOGWARN("Tried to disable isolation but it was not enabled in the first place");
            }
            isolationEnabled_ = false;
            LOGINFO("Device is no longer isolated");
        }

        Common::Telemetry::TelemetryObject isolationEnabledTelemetry;
        isolationEnabledTelemetry.set(Common::Telemetry::TelemetryValue(isolationEnabled_));
        Common::Telemetry::TelemetryHelper::getInstance().set(
                DeviceIsolation::Telemetry::currentlyActiveKey, isolationEnabledTelemetry, true);

        LOGDEBUG("Finished processing action");
    }

    void PluginAdapter::processPolicy(const std::string& appId, const std::string& policyXml)
    {
        LOGDEBUG("Received policy: " << appId);
        if (appId == "NTP")
        {
            ntpPolicy_ = std::make_shared<NTPPolicy>(policyXml);
            LOGINFO("Device Isolation policy applied (" << ntpPolicy_->revId() << ")");
        }
        else
        {
            LOGERROR("Received unexpected policy: " << appId);
        }
    }

    void PluginAdapter::sendStatus()
    {
        auto status = NTPStatus(ntpPolicy_->revId(), isolationEnabled_);
        // Only sends status to Central if xml_without_timestamp changes:
        baseService_->sendStatus("NTP", status.xml(), status.xmlWithoutTimestamp());
    }

} // namespace Plugin