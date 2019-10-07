/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/PluginCallBackHandler.h>
#include <Common/UtilityImpl/Factory.h>
#include <Common/ZMQWrapperApi/IContextSharedPtr.h>
#include "IWatchdogRequest.h"
#include "WatchdogServiceException.h"
#include <functional>

namespace watchdog
{
    namespace watchdogimpl
    {
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
