/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "wdctl/wdctlimpl/Arguments.h"
#include "Action.h"

namespace wdctl
{
    namespace wdctlactions
    {
        class CopyPlugin : public Action
        {
        public:
            explicit CopyPlugin(const wdctl::wdctlimpl::Arguments& args);
            int run() override;
        };
    }
}
