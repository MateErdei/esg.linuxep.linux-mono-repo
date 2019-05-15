/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Proxy.h"

bool Telemetry::TelemetryConfig::Proxy::operator==(const Telemetry::TelemetryConfig::Proxy& rhs) const
{
    return m_url == rhs.m_url && m_port == rhs.m_port && m_authentication == rhs.m_authentication &&
           m_username == rhs.m_username && m_password == rhs.m_password;
}

bool Telemetry::TelemetryConfig::Proxy::operator!=(const Telemetry::TelemetryConfig::Proxy& rhs) const
{
    return !(rhs == *this);
}

bool Telemetry::TelemetryConfig::Proxy::isValid() const
{
    return m_port <= 65535;
}