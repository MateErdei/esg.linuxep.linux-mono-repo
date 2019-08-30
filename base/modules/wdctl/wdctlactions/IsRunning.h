/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ZMQAction.h"
#include "StopAction.h"
namespace wdctl
{
    namespace wdctlactions
    {
        class IsRunning : public StopAction
        {
        public:
            using StopAction::StopAction;
            int run() override;
        };
    } // namespace wdctlactions
} // namespace wdctl
