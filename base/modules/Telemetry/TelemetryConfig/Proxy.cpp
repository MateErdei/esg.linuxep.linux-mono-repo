/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Proxy.h"

using namespace Telemetry::TelemetryConfig;

bool Proxy::operator==(const Proxy& rhs) const
{
    if (this == &rhs)
    {
        return true;
    }

    return m_url == rhs.m_url && m_port == rhs.m_port && m_authentication == rhs.m_authentication &&
           m_username == rhs.m_username && m_password == rhs.m_password;
}

bool Proxy::operator!=(const Proxy& rhs) const
{
    return !(rhs == *this);
}

bool Proxy::isValidProxy() const
{
    if (m_authentication == Proxy::Authentication::none)
    {
        if (!m_username.empty() || !m_password.empty())
        {
            return false;
        }
    }

    if (m_authentication == Proxy::Authentication::basic || m_authentication == Proxy::Authentication::digest)
    {
        if (m_username.empty() || m_password.empty())
        {
            return false;
        }
    }

    return m_port <= MAX_PORT_NUMBER;
}

Proxy::Proxy() : m_url(""), m_port(0), m_authentication(Proxy::Authentication::none), m_username(""), m_password("") {}
