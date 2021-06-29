/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "WaitForUtils.h"

#include <functional>

namespace Common::UtilityImpl
    {
        bool waitFor(double timeToWaitInSeconds, double intervalInSeconds, std::function<bool()> conditionFunction);
    }