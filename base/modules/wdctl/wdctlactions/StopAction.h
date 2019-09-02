/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ZMQAction.h"

namespace wdctl
{
    namespace wdctlactions
    {
        class StopAction : public ZMQAction
        {
        public:
            enum class IsRunningStatus
            {
                IsRunning,
                IsNotRunning,
                Undefined
            };
            explicit StopAction(const wdctl::wdctlarguments::Arguments& args);
            int run() override;
            IsRunningStatus checkIsRunning();
        };
    } // namespace wdctlactions
} // namespace wdctl
