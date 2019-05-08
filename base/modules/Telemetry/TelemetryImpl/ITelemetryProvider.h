/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>

namespace Telemetry
{
    class ITelemetryProvider
    {
        public:
            virtual ~ITelemetryProvider() = default;
            virtual std::string getTelemetry() = 0;
            virtual std::string getName() = 0;
    };

}