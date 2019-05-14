/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Config.h"

bool Telemetry::TelemetryConfig::Config::operator==(const Telemetry::TelemetryConfig::Config & rhs) const
{
    return m_server == rhs.m_server &&
        m_resourceRoute == rhs.m_resourceRoute &&
        m_port == rhs.m_port &&
        m_headers == rhs.m_headers &&
        m_verb == rhs.m_verb &&
        m_proxies == rhs.m_proxies &&
        m_messageRelays == rhs.m_messageRelays &&
        m_externalProcessTimeout == rhs.m_externalProcessTimeout &&
        m_maxJsonSize == rhs.m_maxJsonSize;
}

bool Telemetry::TelemetryConfig::Config::operator!=(const Telemetry::TelemetryConfig::Config & rhs) const
{
    return !(rhs == *this);
}
