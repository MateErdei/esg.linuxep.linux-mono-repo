/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MessageRelay.h"

bool Telemetry::TelemetryConfig::MessageRelay::operator==(const Telemetry::TelemetryConfig::MessageRelay& rhs) const
{
    return m_url == rhs.m_url && m_port == rhs.m_port && m_authentication == rhs.m_authentication &&
           m_username == rhs.m_username && m_password == rhs.m_password && m_certificatePath == rhs.m_certificatePath;
}

bool Telemetry::TelemetryConfig::MessageRelay::operator!=(const Telemetry::TelemetryConfig::MessageRelay& rhs) const
{
    return !(rhs == *this);
}

bool Telemetry::TelemetryConfig::MessageRelay::isValid() const
{
    return static_cast<const Telemetry::TelemetryConfig::Proxy&>(*this).isValid();
}