// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/UtilityImpl/Factory.h"
#include "Common/ZMQWrapperApi/IContext.h"

namespace Common::WatchdogRequest
{
    class IWatchdogRequest
    {
    public:
        virtual ~IWatchdogRequest() = default;
        virtual void requestUpdateService() = 0;
        virtual void requestDiagnoseService() = 0;
    };

    Common::UtilityImpl::Factory<IWatchdogRequest>& factory();
} // namespace Common::WatchdogRequest
