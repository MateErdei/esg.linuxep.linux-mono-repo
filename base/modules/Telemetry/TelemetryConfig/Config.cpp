/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Config.h"

bool Telemetry::TelemetryConfig::Config::operator==(const Telemetry::TelemetryConfig::Config& rhs) const
{
    return m_server == rhs.m_server && m_resourceRoute == rhs.m_resourceRoute && m_port == rhs.m_port &&
           m_headers == rhs.m_headers && m_verb == rhs.m_verb && m_proxies == rhs.m_proxies &&
           m_messageRelays == rhs.m_messageRelays && m_externalProcessTimeout == rhs.m_externalProcessTimeout &&
           m_externalProcessRetries == rhs.m_externalProcessRetries && m_maxJsonSize == rhs.m_maxJsonSize;
}

bool Telemetry::TelemetryConfig::Config::operator!=(const Telemetry::TelemetryConfig::Config& rhs) const
{
    return !(rhs == *this);
}

bool Telemetry::TelemetryConfig::Config::isValid() const
{
    for (auto& messageRelay : m_messageRelays)
    {
        if (!messageRelay.isValid())
        {
            return false;
        }
    }

    for (auto& proxy : m_proxies)
    {
        if (!proxy.isValid())
        {
            return false;
        }
    }

    return m_port <= 65535 &&
           (m_verb == Common::HttpSenderImpl::RequestType::GET || m_verb == Common::HttpSenderImpl::RequestType::POST ||
            m_verb == Common::HttpSenderImpl::RequestType::PUT) &&
           m_externalProcessTimeout > 0 && m_maxJsonSize > 0;
}
