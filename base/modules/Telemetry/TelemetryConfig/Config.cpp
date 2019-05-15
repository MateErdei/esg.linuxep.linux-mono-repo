/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Config.h"

using namespace Telemetry::TelemetryConfig;

bool Config::operator==(const Config& rhs) const
{
    if (this == &rhs)
    {
        return true;
    }

    return m_server == rhs.m_server && m_resourceRoute == rhs.m_resourceRoute && m_port == rhs.m_port &&
           m_headers == rhs.m_headers && m_verb == rhs.m_verb && m_proxies == rhs.m_proxies &&
           m_messageRelays == rhs.m_messageRelays && m_externalProcessTimeout == rhs.m_externalProcessTimeout &&
           m_externalProcessRetries == rhs.m_externalProcessRetries && m_maxJsonSize == rhs.m_maxJsonSize;
}

bool Config::operator!=(const Config& rhs) const
{
    return !(rhs == *this);
}

bool Config::isValid() const
{
    for (const auto& messageRelay : m_messageRelays)
    {
        if (!messageRelay.isValidMessageRelay())
        {
            return false;
        }
    }

    for (const auto& proxy : m_proxies)
    {
        if (!proxy.isValidProxy())
        {
            return false;
        }
    }

    return m_port <= m_maxPortNumber &&
           (m_verb == Common::HttpSenderImpl::RequestType::GET || m_verb == Common::HttpSenderImpl::RequestType::POST ||
            m_verb == Common::HttpSenderImpl::RequestType::PUT) &&
           m_externalProcessTimeout > 0 && m_maxJsonSize > 0;
}

Config::Config() :
    m_port(0),
    m_verb(Common::HttpSenderImpl::RequestType::PUT),
    m_externalProcessTimeout(0),
    m_externalProcessRetries(0),
    m_maxJsonSize(0)
{
}
