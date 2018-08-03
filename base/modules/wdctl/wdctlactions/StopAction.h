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
            explicit StopAction(const wdctl::wdctlimpl::Arguments& args);
            int run() override;

        };
    }
}


