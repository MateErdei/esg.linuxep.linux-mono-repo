// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "SulDownloader/suldownloaderdata/ConfigurationData.h"
namespace SulDownloader
{
    struct SUSRequestParameters
    {
        std::string product;
        bool isServer = false;
        std::string platformToken;
        std::vector<Common::Policy::ProductSubscription> subscriptions;
        std::string tenantId;
        std::string deviceId;
        std::string baseUrl;
        std::string jwt;
        int timeoutSeconds = 0;
        Common::Policy::Proxy proxy;
        Common::Policy::ESMVersion esmVersion;
    };
}