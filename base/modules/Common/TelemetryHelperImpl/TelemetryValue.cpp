/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryValue.h"

#include <sstream>
#include <stdexcept>

namespace Common::Telemetry
{
    TelemetryValue::TelemetryValue() {}

    TelemetryValue::TelemetryValue(const std::string& value) : m_value(value) {}

    TelemetryValue::TelemetryValue(const bool value) : m_value(value) {}

    TelemetryValue::TelemetryValue(const long value) : m_value(value) {}

    TelemetryValue::TelemetryValue(const unsigned long value) : m_value(value) {}

    TelemetryValue::TelemetryValue(const double value) : m_value(value) {}

    TelemetryValue::TelemetryValue(const char* value) : m_value(std::string(value)) {}

    void TelemetryValue::set(const long value) { m_value = value; }

    void TelemetryValue::set(unsigned long value) { m_value = value; }

    void TelemetryValue::set(double value) { m_value = value; }

    void TelemetryValue::set(const bool value) { m_value = value; }

    void TelemetryValue::set(const std::string& value) { m_value = value; }

    void TelemetryValue::set(const char* value) { m_value = std::string(value); }

    long TelemetryValue::getInteger() const
    {
        checkType(Type::integer_type);
        return std::get<long>(m_value);
    }

    unsigned long TelemetryValue::getUnsignedInteger() const
    {
        checkType(Type::unsigned_integer_type);
        return std::get<unsigned long>(m_value);
    }

    double TelemetryValue::getDouble() const
    {
        checkType(Type::double_type);
        return std::get<double>(m_value);
    }

    bool TelemetryValue::getBoolean() const
    {
        checkType(Type::boolean_type);
        return std::get<bool>(m_value);
    }

    std::string TelemetryValue::getString() const
    {
        checkType(Type::string_type);
        return std::get<std::string>(m_value);
    }

    TelemetryValue::Type TelemetryValue::getType() const { return static_cast<Type>(m_value.index()); }

    bool TelemetryValue::operator==(const TelemetryValue& rhs) const { return m_value == rhs.m_value; }

    bool TelemetryValue::operator!=(const TelemetryValue& rhs) const { return !(rhs == *this); }

    void TelemetryValue::checkType(Type expectedType) const
    {
        if (getType() != expectedType)
        {
            std::stringstream msg;
            msg << "Telemetry value does not contain the expected type. Expected: " << static_cast<int>(expectedType)
                << " Actual: " << m_value.index();
            throw std::logic_error(msg.str());
        }
    }
}