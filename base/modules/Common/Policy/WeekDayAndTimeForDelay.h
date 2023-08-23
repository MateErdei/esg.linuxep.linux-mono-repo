// Copyright 2020-2023 Sophos Limited. All rights reserved.
#pragma once

namespace Common::Policy
{
    // Settings for delayed updates
    struct WeekDayAndTimeForDelay
    {
        bool enabled = false;
        int weekDay = 0;
        int hour = 0;
        int minute = 0;
        // seconds is not used when setting up delayed updates
    };
} // namespace Common::Policy
