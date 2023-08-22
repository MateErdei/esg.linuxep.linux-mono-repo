// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/PluginApi/ApiException.h"
#include "Common/UtilityImpl/Factory.h"

namespace watchdog::watchdogimpl
{
    class IWatchdogRequest
    {
    public:
        virtual ~IWatchdogRequest() = default;
        virtual void requestUpdateService() = 0;
        virtual void requestDiagnoseService() = 0;
    };

    Common::UtilityImpl::Factory<IWatchdogRequest>& factory();
} // namespace watchdog::watchdogimpl
