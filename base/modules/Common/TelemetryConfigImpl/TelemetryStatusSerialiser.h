// Copyright 2024 Sophos Limited. All rights reserved.
#pragma once

#include "TelemetryStatus.h"

namespace Common::TelemetryConfigImpl
{
    class TelemetryStatusSerialiser
    {
    public:
        static std::string serialise(const TelemetryStatus& config);
        static TelemetryStatus deserialise(const std::string& jsonString);
    };

} // namespace TelemetryConfigImpl