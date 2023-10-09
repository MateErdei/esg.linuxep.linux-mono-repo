// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Config.h"

#include "Common/UtilityImpl/StringUtils.h"

using namespace Common::TelemetryConfigImpl;

Config::Config() :
    m_port(0),
    m_verb(VERB_PUT),
    m_interval(0),
    m_externalProcessWaitTime(DEFAULT_PROCESS_WAIT_TIME),
    m_externalProcessWaitRetries(DEFAULT_PROCESS_WAIT_RETRIES),
    m_maxJsonSize(DEFAULT_MAX_JSON_SIZE),
    m_pluginSendReceiveTimeout(DEFAULT_PLUGIN_SEND_RECEIVE_TIMEOUT),
    m_pluginConnectionTimeout(DEFAULT_PLUGIN_CONNECTION_TIMEOUT)
{
}

Config Config::buildExeConfigFromTelemetryConfig(
    const Config& supplementaryConfig,
    const std::string& telemetryHost,
    const std::string& resourceName)
{
    Config exeConfig;
    exeConfig.setServer(telemetryHost);
    exeConfig.setPort(supplementaryConfig.getPort());
    exeConfig.setVerb(supplementaryConfig.getVerb());
    exeConfig.setHeaders(supplementaryConfig.getHeaders());

    std::string resourceRoot = supplementaryConfig.getResourceRoot();

    if (resourceRoot.back() != '/')
    {
        resourceRoot.push_back('/');
    }

    exeConfig.setResourcePath(resourceRoot + resourceName);

    return exeConfig;
}

bool Config::operator==(const Config& rhs) const
{
    if (this == &rhs)
    {
        return true;
    }

    return m_server == rhs.m_server && m_resourceRoot == rhs.m_resourceRoot && m_resourcePath == rhs.m_resourcePath &&
           m_port == rhs.m_port && m_headers == rhs.m_headers && m_verb == rhs.m_verb && m_interval == rhs.m_interval &&
           m_proxies == rhs.m_proxies && m_messageRelays == rhs.m_messageRelays &&
           m_externalProcessWaitTime == rhs.m_externalProcessWaitTime &&
           m_externalProcessWaitRetries == rhs.m_externalProcessWaitRetries && m_maxJsonSize == rhs.m_maxJsonSize;
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

    if (m_port > MAX_PORT_NUMBER)
    {
        return false;
    }

    if (m_externalProcessWaitTime < MIN_PROCESS_WAIT_TIMEOUT || m_externalProcessWaitTime > MAX_PROCESS_WAIT_TIMEOUT)
    {
        return false;
    }

    if (m_externalProcessWaitRetries > MAX_PROCESS_WAIT_RETRIES)
    {
        return false;
    }

    if (m_maxJsonSize < MIN_MAX_JSON_SIZE || m_maxJsonSize > MAX_MAX_JSON_SIZE)
    {
        return false;
    }

    if (m_verb != VERB_PUT && m_verb != VERB_POST && m_verb != VERB_GET)
    {
        return false;
    }

    if (m_pluginSendReceiveTimeout < MIN_PLUGIN_TIMEOUT || m_pluginSendReceiveTimeout > MAX_PLUGIN_TIMEOUT)
    {
        return false;
    }

    if (m_pluginConnectionTimeout < MIN_PLUGIN_TIMEOUT || m_pluginConnectionTimeout > MAX_PLUGIN_TIMEOUT)
    {
        return false;
    }

    return true;
}

const std::string& Config::getServer() const
{
    return m_server;
}

void Config::setServer(const std::string& server)
{
    m_server = server;
}

const std::string& Config::getResourceRoot() const
{
    return m_resourceRoot;
}

void Config::setResourceRoot(const std::string& resourceRoot)
{
    m_resourceRoot = resourceRoot;
}

void Config::setResourcePath(const std::string& resourcePath)
{
    m_resourcePath = resourcePath;
}

const std::string& Config::getResourcePath() const
{
    return m_resourcePath;
}

unsigned int Config::getPort() const
{
    return m_port;
}

void Config::setPort(unsigned int port)
{
    m_port = port;
}

const Common::HttpRequests::Headers& Config::getHeaders() const
{
    return m_headers;
}

void Config::setHeaders(const Common::HttpRequests::Headers& headers)
{
    m_headers = headers;
}

std::string Config::getVerb() const
{
    return m_verb;
}

void Config::setVerb(const std::string& verb)
{
    m_verb = verb;
}

unsigned int Config::getInterval() const
{
    return m_interval;
}

void Config::setInterval(unsigned int interval)
{
    m_interval = interval;
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

unsigned int Config::getExternalProcessWaitTime() const
{
    return m_externalProcessWaitTime;
}

void Config::setExternalProcessWaitTime(unsigned int externalProcessWaitTime)
{
    m_externalProcessWaitTime = externalProcessWaitTime;
}

unsigned int Config::getExternalProcessWaitRetries() const
{
    return m_externalProcessWaitRetries;
}

void Config::setExternalProcessWaitRetries(unsigned int externalProcessWaitRetries)
{
    m_externalProcessWaitRetries = externalProcessWaitRetries;
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
    m_telemetryServerCertificatePath = telemetryServerCertificatePath;
}

int Config::getPluginSendReceiveTimeout() const
{
    return m_pluginSendReceiveTimeout;
}

void Config::setPluginSendReceiveTimeout(int pluginSendReceiveTimeout)
{
    m_pluginSendReceiveTimeout = pluginSendReceiveTimeout;
}

int Config::getPluginConnectionTimeout() const
{
    return m_pluginConnectionTimeout;
}

void Config::setPluginConnectionTimeout(int pluginConnectionTimeout)
{
    m_pluginConnectionTimeout = pluginConnectionTimeout;
}
