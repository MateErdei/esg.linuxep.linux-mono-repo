// Copyright 2024 Sophos All rights reserved.

#pragma once

namespace Plugin
{
    enum class IsolateResult
    {
        SUCCESS,
        // The rules were not present when trying to remove them
        RULES_NOT_PRESENT,
        // The sophos rules table already exists in nft ruleset
        RULES_ALREADY_PRESENT,
        FAILED
    };
}