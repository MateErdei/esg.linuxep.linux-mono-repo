// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "MCSHttpClient.h"
#include "ConfigOptions.h"

#include <json.hpp>

#include <chrono>
#include <string>
#include <map>

namespace MCS
{
    class MCSApiCalls
    {
        public:
            std::map<std::string,std::string> getAuthenticationInfo(MCSHttpClient client);

            bool registerEndpoint(
                MCSHttpClient& client,
                MCS::ConfigOptions& configOptions,
                const std::string& statusXml,
                const std::string& proxy
            );

            std::string preregisterEndpoint(
                MCSHttpClient& client,
                MCS::ConfigOptions& registerConfig,
                const std::string& statusXml,
                const std::string& proxy
            );

            using duration_t = std::chrono::seconds;

            /**
             * Synchronous method to get a policy
             * @param client
             * @param appId
             * @param policyId
             * @param timeout
             * @return PolicyXML
             */
            std::optional<std::string> getPolicy(
                MCSHttpClient& client,
                const std::string& appId="ALC",
                int policyId=1,
                duration_t timeout=std::chrono::seconds{600},
                duration_t pollInterval=std::chrono::seconds{20});

    };
}