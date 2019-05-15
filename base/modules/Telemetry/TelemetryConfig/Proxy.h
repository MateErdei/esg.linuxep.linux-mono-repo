/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Telemetry::TelemetryConfig
{
    class Proxy
    {
    public:
        Proxy();

        enum Authentication
        {
            none = 0,
            basic = 1,
            digest = 2
        };
        std::string m_url;
        unsigned int m_port;
        Authentication m_authentication;
        std::string m_username;
        std::string m_password;

        bool operator==(const Proxy& rhs) const;
        bool operator!=(const Proxy& rhs) const;

        bool isValidProxy() const;
    };
} // namespace Telemetry::TelemetryConfig
