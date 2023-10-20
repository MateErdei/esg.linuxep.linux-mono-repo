// Copyright 2023 Sophos All rights reserved.

#pragma once

#include "common/StoppableSleeper.h"

namespace unixsocket
{
    using IStoppableSleeper = common::StoppableSleeper;
    using IStoppableSleeperSharedPtr = common::IStoppableSleeperSharedPtr;
    using duration_t = IStoppableSleeper::duration_t;
    static constexpr duration_t DEFAULT_CLIENT_SLEEP_TIME = std::chrono::seconds{1};
}
