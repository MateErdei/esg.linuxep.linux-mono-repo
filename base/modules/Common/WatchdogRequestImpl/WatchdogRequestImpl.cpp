// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/PluginCommunication/IPluginCommunicationException.h"
#include "Common/PluginCommunicationImpl/PluginProxy.h"
#include "Common/UtilityImpl/Factory.h"
#include "Common/WatchdogConstants/WatchdogConstants.h"
#include "Common/WatchdogRequest/IWatchdogRequest.h"
#include "Common/WatchdogRequest/WatchdogServiceException.h"
#include "Common/ZMQWrapperApi/IContext.h"

namespace Common::WatchdogRequestImpl
{
    void requestUpdateService(Common::ZMQWrapperApi::IContext& context)
    {
        LOGINFO("Request Watchdog to trigger Update service.");
        try
        {
            auto requester = context.getRequester();
            Common::PluginApiImpl::PluginResourceManagement::setupRequester(
                *requester, Common::WatchdogConstants::WatchdogServiceLineName(), 5000, 5000);
            Common::PluginCommunicationImpl::PluginProxy pluginProxy(
                std::move(requester), Common::WatchdogConstants::WatchdogServiceLineName());
            pluginProxy.queueAction("", Common::WatchdogConstants::Actions::TriggerUpdate(), "");
            LOGINFO("Update Acknowledged.");
        }
        catch (Common::PluginCommunication::IPluginCommunicationException& ex)
        {
            std::string exceptionInfo = ex.what();
            if (exceptionInfo == Common::WatchdogRequest::UpdateServiceReportError::ErrorReported())
            {
                throw Common::WatchdogRequest::UpdateServiceReportError();
            }
            else
            {
                LOGERROR(exceptionInfo);
            }
            throw Common::WatchdogRequest::WatchdogServiceException("Service Unavailable");
        }
        catch (std::exception& ex)
        {
            LOGERROR("Unexpected exception thrown while requesting update: " << ex.what());
            assert(false); // not expecting other type of exception.
            throw Common::WatchdogRequest::WatchdogServiceException(ex.what());
        }
    }

    void requestDiagnoseService(Common::ZMQWrapperApi::IContext& context)
    {
        LOGINFO("Request Watchdog to trigger Diagnose service.");
        try
        {
            auto requester = context.getRequester();
            Common::PluginApiImpl::PluginResourceManagement::setupRequester(
                *requester, Common::WatchdogConstants::WatchdogServiceLineName(), 5000, 5000);
            Common::PluginCommunicationImpl::PluginProxy pluginProxy(
                std::move(requester), Common::WatchdogConstants::WatchdogServiceLineName());
            pluginProxy.queueAction("", Common::WatchdogConstants::Actions::TriggerDiagnose(), "");
            LOGINFO("Start Diagnose Acknowledged.");
        }
        catch (Common::PluginCommunication::IPluginCommunicationException& ex)
        {
            std::string exceptionInfo = ex.what();
            if (exceptionInfo == Common::WatchdogRequest::UpdateServiceReportError::ErrorReported())
            {
                throw Common::WatchdogRequest::UpdateServiceReportError();
            }
            else
            {
                LOGERROR(exceptionInfo);
            }
            throw Common::WatchdogRequest::WatchdogServiceException("Service Unavailable");
        }
        catch (std::exception& ex)
        {
            LOGERROR("Unexpected exception thrown while requesting diagnose: " << ex.what());
            assert(false); // not expecting other type of exception.
            throw Common::WatchdogRequest::WatchdogServiceException(ex.what());
        }
    }
} // namespace Common::WatchdogRequestImpl

namespace
{
    class WatchdogRequestImpl : public Common::WatchdogRequest::IWatchdogRequest
    {
    public:
        void requestUpdateService() override
        {
            auto context = Common::ZMQWrapperApi::createContext();
            Common::WatchdogRequestImpl::requestUpdateService(*context);
        }

        void requestDiagnoseService() override
        {
            auto context = Common::ZMQWrapperApi::createContext();
            Common::WatchdogRequestImpl::requestDiagnoseService(*context);
        }
    };
} // namespace

namespace Common::WatchdogRequest
{
    Common::UtilityImpl::Factory<IWatchdogRequest>& factory()
    {
        static Common::UtilityImpl::Factory<IWatchdogRequest> theFactory{ []() {
            return std::make_unique<::WatchdogRequestImpl>();
        } };
        return theFactory;
    }
} // namespace Common::WatchdogRequest
