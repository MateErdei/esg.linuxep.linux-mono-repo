/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Config.h"

#include <string>

namespace Telemetry::TelemetryConfigImpl
{
    class Serialiser
    {
    public:
        static std::string serialise(const Config& config);
        static Config deserialise(const std::string& jsonString);
    };

} // namespace Telemetry::TelemetryExeConfigImpl
