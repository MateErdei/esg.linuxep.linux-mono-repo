/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "WatchdogServiceLine.h"

#include "Logger.h"

#include <Common/PluginApi/IPluginCallbackApi.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/PluginCommunication/IPluginCommunicationException.h>
#include <Common/PluginCommunicationImpl/PluginProxy.h>
#include <Common/Process/IProcess.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/Factory.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/UtilityImpl/ConfigException.h>

namespace
{
    void runTriggerUpdate()
    {
        auto process = Common::Process::createProcess();
        process->setOutputLimit(1000);
        process->exec("/bin/systemctl", { "restart", "sophos-spl-update.service" });
        process->waitUntilProcessEnds();
        std::string output = process->output();
        int exitCode = process->exitCode();
        if (exitCode != 0)
        {
            LOGWARN("Trigger reported failure. ExitCode(" << exitCode << ") Output: " << output);
            throw watchdog::watchdogimpl::UpdateServiceReportError();
        }
        else
        {
            LOGSUPPORT(output);
        }
    }

    class WDServiceCallBack : public Common::PluginApi::IPluginCallbackApi
    {
    public:
        static const std::string& TriggerUpdate()
        {
            static const std::string trigger{ "TriggerUpdate" };
            return trigger;
        }
        WDServiceCallBack(std::function<std::vector<std::string>(void)> getPluginListFunc) :
                m_getListOfPluginsFunc(std::move(getPluginListFunc))
        {}


        ~WDServiceCallBack() = default;

        void applyNewPolicy(const std::string&) override { LOGWARN("NotSupported: Received apply new policy"); }

        /**
         * Current action available is ontly TriggerUpdate which triggers the sophos-spl-update.
         * @throw UpdateServiceReportError if the request either is reported as failure or even the service is not
         * available action.
         */
        void queueAction(const std::string& action) override
        {
            if (action == TriggerUpdate())
            {
                LOGSUPPORT("Trigger sophos-spl-update service");

                runTriggerUpdate();
                LOGSUPPORT("Trigger sophos-spl-update service done");
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

            for (auto pluginName: m_getListOfPluginsFunc())
            {
                Common::Telemetry::TelemetryHelper::getInstance().increment(
                        watchdog::watchdogimpl::createUnexpectedRestartTelemetryKeyFromPluginName(pluginName),
                        0UL
                        );
            }

            return Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
        }

        std::function<std::vector<std::string>(void)> m_getListOfPluginsFunc;
    };

    class WatchdogRequestImpl : public watchdog::watchdogimpl::IWatchdogRequest
    {
    public:
        void requestUpdateService() override
        {
            return watchdog::watchdogimpl::WatchdogServiceLine::requestUpdateService();
        }
    };

} // namespace
namespace watchdog
{
    namespace watchdogimpl
    {
        void WatchdogServiceLine::requestUpdateService(Common::ZMQWrapperApi::IContext& context)
        {
            LOGINFO("Request Watchdog to trigger Update service.");
            try
            {
                auto requester = context.getRequester();
                Common::PluginApiImpl::PluginResourceManagement::setupRequester(
                    *requester, WatchdogServiceLineName(), 5000, 5000);
                Common::PluginCommunicationImpl::PluginProxy pluginProxy(
                    std::move(requester), WatchdogServiceLineName());
                pluginProxy.queueAction("", WDServiceCallBack::TriggerUpdate(), "");
                LOGINFO("Update Acknowledged.");
            }
            catch (Common::PluginCommunication::IPluginCommunicationException& ex)
            {
                std::string exceptionInfo = ex.what();
                if (exceptionInfo == UpdateServiceReportError::ErrorReported())
                {
                    throw UpdateServiceReportError();
                }
                else
                {
                    LOGERROR(exceptionInfo);
                }
                throw WatchdogServiceException("Service Unavailable");
            }
            catch (std::exception& ex)
            {
                LOGERROR("Unexpected exception thrown while requesting update: " << ex.what());
                assert(false); // not expecting other type of exception.
                throw WatchdogServiceException(ex.what());
            }
        }

        void WatchdogServiceLine::requestUpdateService()
        {
            auto context = Common::ZMQWrapperApi::createContext();
            requestUpdateService(*context);
        }

        WatchdogServiceLine::WatchdogServiceLine(Common::ZMQWrapperApi::IContextSharedPtr context, std::function<std::vector<std::string>(void)> getPluginListFunc) : m_context(context)
        {
            try
            {
                auto replier = m_context->getReplier();
                Common::PluginApiImpl::PluginResourceManagement::setupReplier(*replier, WatchdogServiceLineName(), 5000, 5000);
                std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback{ new WDServiceCallBack(getPluginListFunc) };
                m_pluginHandler.reset(new Common::PluginApiImpl::PluginCallBackHandler(
                        WatchdogServiceLineName(),
                        std::move(replier),
                        std::move(pluginCallback),
                        Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY::DONOTARM));
                m_pluginHandler->start();

            }catch ( std::exception & ex)
            {
                throw Common::UtilityImpl::ConfigException( "WatchdogService", ex.what());
            }
        }

        WatchdogServiceLine::~WatchdogServiceLine()
        {
            if (m_pluginHandler)
            {
                m_pluginHandler->stop();
            }
        }

        Common::UtilityImpl::Factory<IWatchdogRequest>& factory()
        {
            static Common::UtilityImpl::Factory<IWatchdogRequest> theFactory{ []() {
                return std::unique_ptr<IWatchdogRequest>(new WatchdogRequestImpl());
            } };
            return theFactory;
        }

        std::string createUnexpectedRestartTelemetryKeyFromPluginName(const std::string& pluginName)
        {
            std::stringstream telemetryMessage;
            telemetryMessage << pluginName << "-unexpected-restarts";
            return telemetryMessage.str();
        }

    } // namespace watchdogimpl
} // namespace watchdog