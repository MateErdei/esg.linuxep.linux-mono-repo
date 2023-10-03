// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

namespace JournalerCommon
{
    static constexpr const int DEFAULT_QUEUE_SLEEP_INTERVAL_MS = 50000;
    static constexpr const int DEFAULT_READ_LOOP_TIMEOUT_MS = 50000;
    constexpr long MAX_PING_TIMEOUT_SECONDS = std::max(
            DEFAULT_QUEUE_SLEEP_INTERVAL_MS / 1000,
            DEFAULT_READ_LOOP_TIMEOUT_MS / 1000) +
                                              3;
} // namespace JournalerCommon