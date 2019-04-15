/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <variant>
#include "TelemetryNode.h"

namespace Common::Telemetry
{
    class TelemetryValue : public TelemetryNode
    {
    public:
        TelemetryValue();
        ~TelemetryValue() override = default;

        void set(int value);
        void set(bool value);
        void set(const std::string& value);

        int getInteger();
        bool getBoolean();
        std::string getString();

    private:
        std::variant<int, bool, std::string> m_value;

    };
}
