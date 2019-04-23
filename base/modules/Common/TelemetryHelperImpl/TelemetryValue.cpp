/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <stdexcept>
#include "TelemetryValue.h"

namespace Common::Telemetry
{

    TelemetryValue::TelemetryValue()
        : m_valueType(ValueType::unset)
    {
    }

    TelemetryValue::TelemetryValue(const std::string& value)
        : m_value(value), m_valueType(ValueType::string_type)
    {
    }

    TelemetryValue::TelemetryValue(const bool value)
        : m_value(value), m_valueType(ValueType::boolean_type)
    {
    }

    TelemetryValue::TelemetryValue(const int value)
        : m_value(value), m_valueType(ValueType::integer_type)
    {
    }

    TelemetryValue::TelemetryValue(const unsigned int value)
            : m_value(value), m_valueType(ValueType::unsigned_integer_type)
    {
    }

    TelemetryValue::TelemetryValue(const char* value)
        : m_value(std::string(value)), m_valueType(ValueType::string_type)
    {
    }

    void TelemetryValue::set(const int value)
    {
        m_value = value;
        m_valueType = ValueType::integer_type;
    }

    void TelemetryValue::set(unsigned int value)
    {
        m_value = value;
        m_valueType = ValueType::unsigned_integer_type;
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
        checkType(ValueType::integer_type);
        return std::get<int>(m_value);
    }

    unsigned int TelemetryValue::getUnsignedInteger() const
    {
        checkType(ValueType::unsigned_integer_type);
        return std::get<unsigned int>(m_value);
    }

    bool TelemetryValue::getBoolean() const
    {
        checkType(ValueType::boolean_type);
        return std::get<bool>(m_value);
    }

    std::string TelemetryValue::getString() const
    {
        checkType(ValueType::string_type);
        return std::get<std::string>(m_value);
    }

    TelemetryValue::ValueType TelemetryValue::getValueType() const
    {
        return m_valueType;
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

    void TelemetryValue::checkType(ValueType expectedType) const
    {
        if(m_valueType != expectedType)
        {
            throw std::invalid_argument("bad telem value type");
        }
    }
}