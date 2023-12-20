// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once


#include <string>
#include <vector>

#include "IsolationExclusion.h"

namespace Plugin
{
    class NftWrapper
    {
        static constexpr auto TABLE_NAME = "sophos_device_isolation";

    public:
        enum class IsolateResult
        {
            SUCCESS,
            WARN,
            FAILED
        };

        // TODO LINUXDAR-7964 Remove [[maybe_unused]]
        // Isolate the endpoint and allow the specified IP exclusions
        static IsolateResult
        applyIsolateRules([[maybe_unused]] const std::vector<Plugin::IsolationExclusion>& allowList);

        // Flush all Sophos isolation rules
        static IsolateResult clearIsolateRules();

    };
} // namespace Plugin
