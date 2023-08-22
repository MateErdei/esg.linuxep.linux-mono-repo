/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "StopAction.h"
#include "ZMQAction.h"
namespace wdctl::wdctlactions
{
    class IsRunning : public StopAction
    {
    public:
        using StopAction::StopAction;
        int run() override;
    };
} // namespace wdctl::wdctlactions
