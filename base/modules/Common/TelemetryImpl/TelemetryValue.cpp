/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryValue.h"

namespace Common::Telemetry
{

    TelemetryValue::TelemetryValue()
        : TelemetryNode(NodeType::value)
    {
    }

    void TelemetryValue::set(const int value)
    {
        m_value = value;
    }

    void TelemetryValue::set(const bool value)
    {
        m_value = value;
    }

    void TelemetryValue::set(const std::string& value)
    {
        m_value = value;
    }

    int TelemetryValue::getInteger()
    {
        return std::get<int>(m_value);
    }

    bool TelemetryValue::getBoolean()
    {
        return std::get<bool>(m_value);
    }

    std::string TelemetryValue::getString()
    {
        return std::get<std::string>(m_value);
    }
}