/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <variant>

namespace Common::Telemetry
{
    class TelemetryValue
    {
    public:
        TelemetryValue();
        explicit TelemetryValue(const std::string& value);
        explicit TelemetryValue(const bool value);
        explicit TelemetryValue(const int value);
        explicit TelemetryValue(const unsigned int value);
        explicit TelemetryValue(const char* value);
        ~TelemetryValue() = default;

        enum class ValueType
        {
            unset,
            integer_type,
            unsigned_integer_type,
            boolean_type,
            string_type
        };

        void set(int value);
        void set(unsigned int value);
        void set(bool value);
        void set(const std::string& value);
        void set(const char* value);

        ValueType getValueType() const;
        void checkType(ValueType expectedType) const;

        int getInteger() const;
        unsigned int getUnsignedInteger() const;
        bool getBoolean() const;
        std::string getString() const;

        bool operator==(const TelemetryValue& rhs) const;
        bool operator!=(const TelemetryValue& rhs) const;

    private:
        std::variant<int, unsigned int, bool, std::string> m_value;
        ValueType  m_valueType;

    };
}

