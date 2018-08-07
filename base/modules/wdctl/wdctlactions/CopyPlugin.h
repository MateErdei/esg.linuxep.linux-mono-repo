/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Action.h"

#include <wdctl/wdctlimpl/Arguments.h>

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
