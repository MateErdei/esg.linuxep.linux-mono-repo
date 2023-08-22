/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ZMQAction.h"

namespace wdctl::wdctlactions
{
    class RemoveAction : public ZMQAction
    {
    public:
        explicit RemoveAction(const Action::Arguments& args);

        int run() override;
    };
} // namespace wdctl::wdctlactions
