/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryValue.h"

namespace Common::Telemetry
{

    TelemetryValue::TelemetryValue()
        : TelemetryNode(NodeType::value), m_valueType(ValueType::unset)
    {
    }

    TelemetryValue::TelemetryValue(const std::string& value)
        : TelemetryNode(NodeType::value), m_value(value), m_valueType(ValueType::string_type)
    {
    }

    TelemetryValue::TelemetryValue(const bool value)
        : TelemetryNode(NodeType::value), m_value(value), m_valueType(ValueType::boolean_type)
    {
    }

    TelemetryValue::TelemetryValue(const int value)
        : TelemetryNode(NodeType::value), m_value(value), m_valueType(ValueType::integer_type)
    {
    }

    TelemetryValue::TelemetryValue(const char* value)
        : TelemetryNode(NodeType::value), m_value(std::string(value)), m_valueType(ValueType::string_type)
    {
    }

    void TelemetryValue::set(const int value)
    {
        m_value = value;
        m_valueType = ValueType::integer_type;
    }

    void TelemetryValue::set(const bool value)
    {
        m_value = value;
        m_valueType = ValueType::boolean_type;
    }

    void TelemetryValue::set(const std::string& value)
    {
        m_value = value;
        m_valueType = ValueType::string_type;
    }

    void TelemetryValue::set(const char* value)
    {
        m_value = std::string(value);
        m_valueType = ValueType::string_type;
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

    ValueType TelemetryValue::getValueType() const
    {
        return m_valueType;
    }
}