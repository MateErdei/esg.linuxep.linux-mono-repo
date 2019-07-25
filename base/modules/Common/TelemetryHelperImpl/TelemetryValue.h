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
        // TelemetryValue is by default constructed with an int of value 0, this is the first type in the variant.
        TelemetryValue();
        explicit TelemetryValue(const std::string& value);
        explicit TelemetryValue(const bool value);
        explicit TelemetryValue(const long value);
        explicit TelemetryValue(const unsigned long value);
        explicit TelemetryValue(const double value);
        explicit TelemetryValue(const char* value);
        ~TelemetryValue() = default;

        // Must be in same order as the types in the m_value variant.
        enum class Type
        {
            integer_type,
            unsigned_integer_type,
            double_type,
            boolean_type,
            string_type
        };

        void set(long value);
        void set(unsigned long value);
        void set(bool value);
        void set(double value);
        void set(const std::string& value);
        void set(const char* value);

        Type getType() const;
        void checkType(Type expectedType) const;

        long getInteger() const;
        unsigned long getUnsignedInteger() const;
        double getDouble() const;
        bool getBoolean() const;
        std::string getString() const;

        bool operator==(const TelemetryValue& rhs) const;
        bool operator!=(const TelemetryValue& rhs) const;

    private:
        std::variant<long, unsigned long, double, bool, std::string> m_value;
    };
} // namespace Common::Telemetry
