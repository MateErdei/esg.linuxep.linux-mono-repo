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

    TelemetryValue::TelemetryValue(const std::string& str, const std::string& value)
        : TelemetryNode(str, NodeType::value), m_value(value), m_valueType(ValueType::string_type)
    {
    }

    TelemetryValue::TelemetryValue(const std::string& str, const bool value)
        : TelemetryNode(str, NodeType::value), m_value(value), m_valueType(ValueType::boolean_type)
    {
    }

    TelemetryValue::TelemetryValue(const std::string& str, const int value)
        : TelemetryNode(str, NodeType::value), m_value(value), m_valueType(ValueType::integer_type)
    {
    }

    TelemetryValue::TelemetryValue(const std::string& str, const char* value)
        : TelemetryNode(str, NodeType::value), m_value(std::string(value)), m_valueType(ValueType::string_type)
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

    int TelemetryValue::getInteger() const
    {
        return std::get<int>(m_value);
    }

    bool TelemetryValue::getBoolean() const
    {
        return std::get<bool>(m_value);
    }

    std::string TelemetryValue::getString() const
    {
        return std::get<std::string>(m_value);
    }

    ValueType TelemetryValue::getValueType() const
    {
        return m_valueType;
    }

    TelemetryValue::TelemetryValue(const std::string& str)
        : TelemetryNode(str, NodeType::value), m_valueType(ValueType::unset)
    {

    }

    bool TelemetryValue::operator==(const TelemetryValue& rhs) const
    {
        return m_value == rhs.m_value &&
               m_valueType == rhs.m_valueType;
    }

    bool TelemetryValue::operator!=(const TelemetryValue& rhs) const
    {
        return !(rhs == *this);
    }
}