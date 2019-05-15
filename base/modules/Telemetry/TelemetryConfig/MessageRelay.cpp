/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MessageRelay.h"

using namespace Telemetry::TelemetryConfig;

bool MessageRelay::operator==(const MessageRelay& rhs) const
{
    if (this == &rhs)
    {
        return true;
    }

    return Proxy::operator==(rhs) && m_id == rhs.m_id && m_priority == rhs.m_priority;
}

bool MessageRelay::operator!=(const MessageRelay& rhs) const
{
    return !(rhs == *this);
}

bool MessageRelay::isValidMessageRelay() const
{
    return isValidProxy();
}

MessageRelay::MessageRelay() : m_id(""), m_priority(0) {}
