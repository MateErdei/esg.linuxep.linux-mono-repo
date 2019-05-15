/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MessageRelay.h"

using namespace Telemetry::TelemetryConfig;

bool MessageRelay::operator==(const MessageRelay& rhs) const
{
    return m_url == rhs.m_url && m_port == rhs.m_port && m_authentication == rhs.m_authentication &&
           m_username == rhs.m_username && m_password == rhs.m_password && m_certificatePath == rhs.m_certificatePath;
}

bool MessageRelay::operator!=(const MessageRelay& rhs) const
{
    return !(rhs == *this);
}

bool MessageRelay::isValidMessageRelay() const
{
    return isValidProxy();
}

MessageRelay::MessageRelay() :
    m_certificatePath("")
{
}
