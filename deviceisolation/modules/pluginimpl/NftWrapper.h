// Copyright 2023-2024 Sophos Limited. All rights reserved.

#pragma once

#include <string>
#include <vector>

#include "IsolationExclusion.h"

#include "deviceisolation/modules/plugin/INftWrapper.h"

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

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

    TEST_PUBLIC:
        /**
         * Timeout for nft commands in deci-seconds (1/10 second)
         * https://en.wikipedia.org/wiki/Metric_prefix
         */
        static constexpr int NFT_TIMEOUT_DS = 50;

        static std::string createIsolateRules(const std::vector<Plugin::IsolationExclusion>& allowList);
        static std::string createIsolateRules(const std::vector<Plugin::IsolationExclusion>& allowList, gid_t gid);

    };
} // namespace Plugin
