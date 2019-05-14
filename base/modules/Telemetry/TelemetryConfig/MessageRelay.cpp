/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MessageRelay.h"
bool Telemetry::TelemetryConfig::MessageRelay::operator==(const Telemetry::TelemetryConfig::MessageRelay& rhs) const
{
    return static_cast<const Telemetry::TelemetryConfig::Proxy&>(*this) ==
               static_cast<const Telemetry::TelemetryConfig::Proxy&>(rhs) &&
           m_certificatePath == rhs.m_certificatePath;
}

bool Telemetry::TelemetryConfig::MessageRelay::operator!=(const Telemetry::TelemetryConfig::MessageRelay& rhs) const
{
    return !(rhs == *this);
}
