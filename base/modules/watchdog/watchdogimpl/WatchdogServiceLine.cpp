// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "WatchdogServiceLine.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/PluginApi/IPluginCallbackApi.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessException.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/ConfigException.h"
#include "Common/UtilityImpl/StrError.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/SystemExecutableUtils.h"
#include "Common/WatchdogConstants/WatchdogConstants.h"
#include "Common/WatchdogRequest/WatchdogServiceException.h"
#include "Common/ZMQWrapperApi/IContext.h"

using Common::WatchdogConstants::WatchdogServiceLineName;

namespace
{
    void runTriggerUpdate()
    {
        auto process = Common::Process::createProcess();
        process->setOutputLimit(1000);
        process->exec("/bin/systemctl", { "start", "sophos-spl-update.service" });
        process->waitUntilProcessEnds();
        std::string output = process->output();
        int exitCode = process->exitCode();
        if (exitCode != 0)
        {
            LOGWARN("Trigger reported failure. ExitCode(" << exitCode << ") Output: " << output);
            throw Common::WatchdogRequest::UpdateServiceReportError();
        }
        else
        {
            LOGSUPPORT(output);
        }
    }
    void runTriggerDiagnose()
    {
        auto process = Common::Process::createProcess();
        process->setOutputLimit(1000);
        process->exec("/bin/systemctl", { "start", "sophos-spl-diagnose.service" });
        process->waitUntilProcessEnds();
        std::string output = process->output();
        int exitCode = process->exitCode();
        if (exitCode != 0)
        {
            LOGWARN("Trigger reported failure. ExitCode(" << exitCode << ") Output: " << output);
            throw Common::WatchdogRequest::UpdateServiceReportError();
        }
        else
        {
            LOGSUPPORT(output);
        }
    }
    class WDServiceCallBack : public Common::PluginApi::IPluginCallbackApi
    {
    public:
        WDServiceCallBack(std::function<std::vector<std::string>(void)> getPluginListFunc) :
            m_getListOfPluginsFunc(std::move(getPluginListFunc))
        {
        }

        ~WDServiceCallBack() = default;

        void applyNewPolicy(const std::string& policyXml) override
        {
            LOGWARN("NotSupported: Received apply new policy: " << policyXml);
        }

        /**
         * Current action available is ontly TriggerUpdate which triggers the sophos-spl-update.
         * @throw UpdateServiceReportError if the request either is reported as failure or even the service is not
         * available action.
         */
        void queueAction(const std::string& action) override
        {
            if (action == Common::WatchdogConstants::Actions::TriggerUpdate())
            {
                LOGSUPPORT("Trigger sophos-spl-update service");

                runTriggerUpdate();
                LOGSUPPORT("Trigger sophos-spl-update service done");
                return;
            }
            if (action == Common::WatchdogConstants::Actions::TriggerDiagnose())
            {
                LOGSUPPORT("Trigger sophos-spl-diagnose service");

                runTriggerDiagnose();
                LOGSUPPORT("Trigger sophos-spl-diagnose service done");
                return;
            }
            LOGWARN("Action not supported: " << action);
        }

        void onShutdown() override {}

        Common::PluginApi::StatusInfo getStatus(const std::string& appid) override
        {
            LOGWARN("NotSupported: Received getStatus");
            return Common::PluginApi::StatusInfo{ "", "", appid };
        }

        std::string getTelemetry() override
        {
            LOGSUPPORT("Received get telemetry request");

            for (auto pluginName : m_getListOfPluginsFunc())
            {
                Common::Telemetry::TelemetryHelper::getInstance().increment(
                    watchdog::watchdogimpl::createUnexpectedRestartTelemetryKeyFromPluginName(pluginName), 0UL);
            }
            Common::Telemetry::TelemetryHelper::getInstance().set("health", 0UL);

            try
            {
                Common::Telemetry::TelemetryHelper::getInstance().set("product-disk-usage", getProductDiskUsage());
            }
            catch (Common::Process::IProcessException& e)
            {
                LOGWARN("Command to get product-disk-usage failed due to " << e.what());
            }
            catch (const std::invalid_argument& e)
            {
                LOGWARN("Failed to get product-disk-usage due to " << e.what());
            }
            catch (const std::runtime_error& e)
            {
                LOGWARN("Failed to get product-disk-usage due to " << e.what());
            }

            return Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
        }

        std::string getHealth() override
        {
            LOGDEBUG("Received health request");
            return "{}";
        }

        static unsigned long getProductDiskUsage()
        {
            auto path = Common::UtilityImpl::SystemExecutableUtils::getSystemExecutablePath("du");
            auto process = Common::Process::createProcess();
            const auto& commandExecutablePath = path;
            std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

            process->exec(commandExecutablePath, { "-B", "1000", "-sx", installPath });
            process->setOutputLimit(1024 * 10);

            auto status = process->wait(Common::Process::Milliseconds(1000), 120);
            if (status != Common::Process::ProcessStatus::FINISHED)
            {
                process->kill();
                throw Common::Process::IProcessException("Process execution timed out running 'du'");
            }

            int exitCode = process->exitCode();
            if (exitCode != 0)
            {
                throw Common::Process::IProcessException(
                    "Process execution returned non-zero exit code, 'Exit Code: [" + std::to_string(exitCode) + "] " +
                    Common::UtilityImpl::StrError(exitCode) + "'");
            }

            auto output = process->output();
            std::string strippedOutput =
                Common::UtilityImpl::StringUtils::trim(Common::UtilityImpl::StringUtils::splitString(output, " ")[0]);

            try
            {
                return Common::UtilityImpl::StringUtils::stringToULong(output);
            }
            catch (std::runtime_error& e)
            {
                throw std::runtime_error(
                    "Failed to convert string to unsigned long: " + output + ". Error: " + e.what());
            }
        }

        std::function<std::vector<std::string>(void)> m_getListOfPluginsFunc;
    };
} // namespace
namespace watchdog::watchdogimpl
{
    WatchdogServiceLine::WatchdogServiceLine(
        Common::ZMQWrapperApi::IContextSharedPtr context,
        std::function<std::vector<std::string>(void)> getPluginListFunc) :
        m_context(context)
    {
        try
        {
            Common::Telemetry::TelemetryHelper::getInstance().restore(WatchdogServiceLineName());

            auto replier = m_context->getReplier();
            Common::PluginApiImpl::PluginResourceManagement::setupReplier(
                *replier, WatchdogServiceLineName(), 5000, 5000);
            std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback{ new WDServiceCallBack(
                getPluginListFunc) };
            m_pluginHandler.reset(new Common::PluginApiImpl::PluginCallBackHandler(
                WatchdogServiceLineName(),
                std::move(replier),
                std::move(pluginCallback),
                Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY::DONOTARM));
            m_pluginHandler->start();
        }
        catch (std::exception& ex)
        {
            throw Common::UtilityImpl::ConfigException("WatchdogService", ex.what());
        }
    }

    WatchdogServiceLine::~WatchdogServiceLine()
    {
        if (m_pluginHandler)
        {
            m_pluginHandler->stopAndJoin();
        }
        Common::Telemetry::TelemetryHelper::getInstance().save();
    }

    std::string createUnexpectedRestartTelemetryKeyFromPluginName(const std::string& pluginName)
    {
        std::stringstream telemetryMessage;
        telemetryMessage << pluginName << "-unexpected-restarts";
        return telemetryMessage.str();
    }

    std::string createUnexpectedRestartTelemetryKeyFromPluginNameAndCode(const std::string& pluginName, int code)
    {
        std::stringstream telemetryMessage;
        telemetryMessage << pluginName << "-unexpected-restarts-" << code;
        return telemetryMessage.str();
    }
} // namespace watchdog::watchdogimpl