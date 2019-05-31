/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MessageRelay.h"

using namespace Common::TelemetryExeConfigImpl;

MessageRelay::MessageRelay() : m_priority(0) {}

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

const std::string& MessageRelay::getId() const
{
    return m_id;
}

void MessageRelay::setId(const std::string& id)
{
    m_id = id;
}

int MessageRelay::getPriority() const
{
    return m_priority;
}

void MessageRelay::setPriority(int priority)
{
    m_priority = priority;
}
