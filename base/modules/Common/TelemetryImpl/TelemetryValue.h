/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <variant>
#include "TelemetryNode.h"

namespace Common::Telemetry
{
    enum class ValueType
    {
        unset,
        integer_type,
        boolean_type,
        string_type
    };

    class TelemetryValue : public TelemetryNode
    {
    public:
        TelemetryValue();
        explicit TelemetryValue(const std::string&  value);
        explicit TelemetryValue(const bool value);
        explicit TelemetryValue(const int value);
        explicit TelemetryValue(const char* value);
        ~TelemetryValue() override = default;

        void set(int value);
        void set(bool value);
        void set(const std::string& value);
        void set(const char* value);

        ValueType getValueType() const;

        int getInteger();
        bool getBoolean();
        std::string getString();

    private:
        std::variant<int, bool, std::string> m_value;
        ValueType  m_valueType;

    };
}

