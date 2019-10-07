/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/PluginApi/ApiException.h>
#include <Common/UtilityImpl/Factory.h>

namespace watchdog
{
    namespace watchdogimpl
    {
        class IWatchdogRequest
        {
        public:
            virtual ~IWatchdogRequest() = default;
            virtual void requestUpdateService() = 0;
        };

        Common::UtilityImpl::Factory<IWatchdogRequest>& factory();
    } // namespace watchdogimpl
} // namespace watchdog
