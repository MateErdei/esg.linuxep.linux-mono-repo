/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "MessageRelay.h"
#include "Proxy.h"

#include "Common/HttpSenderImpl/RequestConfig.h"

#include <string>
#include <vector>

namespace Telemetry::TelemetryConfig
{
    class Config
    {
    public:
        Config();

        std::string m_server;
        std::string m_resourceRoute;
        unsigned int m_port{};
        std::vector<std::string> m_headers;
        Common::HttpSenderImpl::RequestType m_verb;
        std::vector<Proxy> m_proxies;
        std::vector<MessageRelay> m_messageRelays;
        unsigned int m_externalProcessTimeout{};
        unsigned int m_externalProcessRetries{};
        unsigned int m_maxJsonSize{};

        bool operator==(const Config& rhs) const;
        bool operator!=(const Config& rhs) const;

        bool isValid() const;
    };
} // namespace Telemetry::TelemetryConfig
