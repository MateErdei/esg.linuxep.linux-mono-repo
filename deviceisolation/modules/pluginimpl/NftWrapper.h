// Copyright 2023-2024 Sophos Limited. All rights reserved.

#pragma once

#include <string>
#include <vector>

#include "IsolationExclusion.h"

#include "deviceisolation/modules/plugin/INftWrapper.h"

namespace Plugin
{
    static constexpr auto TABLE_NAME = "sophos_device_isolation";

    class NftWrapper : public INftWrapper
    {

    public:
        // Isolate the endpoint and allow the specified IP exclusions
        IsolateResult applyIsolateRules(const std::vector<Plugin::IsolationExclusion>& allowList) override;

        // Flush all Sophos isolation rules
        IsolateResult clearIsolateRules() override;

    };
} // namespace Plugin
