/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Action.h"

namespace wdctl::wdctlactions
{
    class CopyPlugin : public Action
    {
    public:
        explicit CopyPlugin(const Action::Arguments& args);
        int run() override;
    };
} // namespace wdctl::wdctlactions