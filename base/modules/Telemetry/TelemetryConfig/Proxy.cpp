/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Proxy.h"

using namespace Telemetry::TelemetryConfig;

Proxy::Proxy() : m_port(0), m_authentication(Proxy::Authentication::none) {}

bool Proxy::operator==(const Proxy& rhs) const
{
    if (this == &rhs)
    {
        return true;
    }

    return m_url == rhs.m_url && m_port == rhs.m_port && m_authentication == rhs.m_authentication &&
           m_username == rhs.m_username && m_obfuscatedPassword == rhs.m_obfuscatedPassword;
}

bool Proxy::operator!=(const Proxy& rhs) const
{
    return !(rhs == *this);
}

bool Proxy::isValidProxy() const
{
    if (m_authentication == Proxy::Authentication::none)
    {
        if (!m_username.empty() || !m_obfuscatedPassword.empty())
        {
            return false;
        }
    }

    if (m_authentication == Proxy::Authentication::basic || m_authentication == Proxy::Authentication::digest)
    {
        if (m_username.empty() || m_obfuscatedPassword.empty())
        {
            return false;
        }
    }

    return m_port <= MAX_PORT_NUMBER;
}

const std::string& Proxy::getUrl() const
{
    return m_url;
}

void Proxy::setUrl(const std::string& url)
{
    m_url = url;
}

unsigned int Proxy::getPort() const
{
    return m_port;
}

void Proxy::setPort(unsigned int port)
{
    m_port = port;
}

Proxy::Authentication Proxy::getAuthentication() const
{
    return m_authentication;
}

void Proxy::setAuthentication(Proxy::Authentication authentication)
{
    m_authentication = authentication;
}

const std::string& Proxy::getUsername() const
{
    return m_username;
}

void Proxy::setUsername(const std::string& username)
{
    m_username = username;
}

const Common::ObfuscationImpl::SecureString Proxy::getDeobfuscatedPassword() const
{
    return Common::ObfuscationImpl::SECDeobfuscate(m_obfuscatedPassword);
}

void Proxy::setPassword(const std::string& obfuscatedPassword)
{
    m_obfuscatedPassword = obfuscatedPassword;
}
const std::string Proxy::getObfuscatedPassword() const
{
    return m_obfuscatedPassword;
}
