/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "MessageRelay.h"
#include "Proxy.h"

#include <string>
#include <vector>

namespace Telemetry::TelemetryConfig
{
    class Config
    {
    public:
        enum HttpVerb
        {
            GET = 0,
            POST = 1,
            PUT = 2
        };

        std::string m_server;
        std::string m_resourceRoute;
        unsigned int m_port;
        std::vector<std::string> m_headers;
        HttpVerb m_verb;
        std::vector<Proxy> m_proxies;
        std::vector<MessageRelay> m_messageRelays;
        unsigned int m_externalProcessTimeout;
        unsigned int m_maxJsonSize;
    };
}
