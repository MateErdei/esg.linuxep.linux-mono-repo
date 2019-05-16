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

    return m_port <= MAX_PORT_NUMBER &&
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
const std::string& Config::getServer() const
{
    return m_server;
}

void Config::setServer(const std::string& server)
{
    m_server = server;
}

const std::string& Config::getResourceRoute() const
{
    return m_resourceRoute;
}

void Config::setResourceRoute(const std::string& resourceRoute)
{
    m_resourceRoute = resourceRoute;
}

unsigned int Config::getPort() const
{
    return m_port;
}

void Config::setPort(unsigned int port)
{
    m_port = port;
}

const std::vector<std::string>& Config::getHeaders() const
{
    return m_headers;
}

void Config::setHeaders(const std::vector<std::string>& headers)
{
    m_headers = headers;
}

Common::HttpSenderImpl::RequestType Config::getVerb() const
{
    return m_verb;
}

void Config::setVerb(Common::HttpSenderImpl::RequestType verb)
{
    m_verb = verb;
}

const std::vector<Proxy>& Config::getProxies() const
{
    return m_proxies;
}

void Config::setProxies(const std::vector<Proxy>& proxies)
{
    m_proxies = proxies;
}

const std::vector<MessageRelay>& Config::getMessageRelays() const
{
    return m_messageRelays;
}

void Config::setMessageRelays(const std::vector<MessageRelay>& messageRelays)
{
    m_messageRelays = messageRelays;
}

unsigned int Config::getExternalProcessTimeout() const
{
    return m_externalProcessTimeout;
}

void Config::setExternalProcessTimeout(unsigned int externalProcessTimeout)
{
    m_externalProcessTimeout = externalProcessTimeout;
}

unsigned int Config::getExternalProcessRetries() const
{
    return m_externalProcessRetries;
}

void Config::setExternalProcessRetries(unsigned int externalProcessRetries)
{
    m_externalProcessRetries = externalProcessRetries;
}

unsigned int Config::getMaxJsonSize() const
{
    return m_maxJsonSize;
}

void Config::setMaxJsonSize(unsigned int maxJsonSize)
{
    m_maxJsonSize = maxJsonSize;
}
const std::string& Config::getTelemetryServerCertificatePath() const
{
    return m_telemetryServerCertificatePath;
}
void Config::setTelemetryServerCertificatePath(const std::string& telemetryServerCertificatePath)
{
    Config::m_telemetryServerCertificatePath = telemetryServerCertificatePath;
}
