/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/PluginCallBackHandler.h>
#include <Common/UtilityImpl/Factory.h>
#include <Common/ZMQWrapperApi/IContextSharedPtr.h>

#include <functional>

namespace watchdog
{
    namespace watchdogimpl
    {
        class WatchdogServiceException : public Common::PluginApi::ApiException
        {
        public:
            using ApiException::ApiException;
        };

        class UpdateServiceReportError : public WatchdogServiceException
        {
        public:
            static std::string ErrorReported() { return "Update service reported error"; }

            UpdateServiceReportError() : WatchdogServiceException(ErrorReported()) {}
        };

        class IWatchdogRequest
        {
        public:
            virtual ~IWatchdogRequest() = default;
            virtual void requestUpdateService() = 0;
        };

        Common::UtilityImpl::Factory<IWatchdogRequest>& factory();

        class WatchdogServiceLine
        {
        public:
            static std::string WatchdogServiceLineName() { return "watchdogservice"; }
            static void requestUpdateService(Common::ZMQWrapperApi::IContext&);
            static void requestUpdateService();
            WatchdogServiceLine(Common::ZMQWrapperApi::IContextSharedPtr);
            ~WatchdogServiceLine();

        private:
            Common::ZMQWrapperApi::IContextSharedPtr m_context;
            std::unique_ptr<Common::PluginApiImpl::PluginCallBackHandler> m_pluginHandler;
        };
    } // namespace watchdogimpl
} // namespace watchdog
