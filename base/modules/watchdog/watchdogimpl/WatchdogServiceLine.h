// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "IWatchdogRequest.h"
#include "WatchdogServiceException.h"

#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApiImpl/PluginCallBackHandler.h"
#include "Common/UtilityImpl/Factory.h"
#include "Common/ZMQWrapperApi/IContextSharedPtr.h"

#include <functional>

namespace watchdog::watchdogimpl
{
    class WatchdogServiceLine
    {
    public:
        WatchdogServiceLine(
            Common::ZMQWrapperApi::IContextSharedPtr,
            std::function<std::vector<std::string>(void)> getPluginListFunc);
        ~WatchdogServiceLine();

        static std::string WatchdogServiceLineName() { return "watchdogservice"; }
        static void requestUpdateService(Common::ZMQWrapperApi::IContext&);
        static void requestUpdateService();
        static void requestDiagnoseService(Common::ZMQWrapperApi::IContext&);
        static void requestDiagnoseService();

    private:
        Common::ZMQWrapperApi::IContextSharedPtr m_context;
        std::unique_ptr<Common::PluginApiImpl::PluginCallBackHandler> m_pluginHandler;
    };

    std::string createUnexpectedRestartTelemetryKeyFromPluginName(const std::string& pluginName);
    std::string createUnexpectedRestartTelemetryKeyFromPluginNameAndCode(const std::string& pluginName, int code);
} // namespace watchdog::watchdogimpl
