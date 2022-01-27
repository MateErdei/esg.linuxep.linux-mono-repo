/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <string>

namespace Telemetry
{
    class TelemetryUtils
    {
    public:
        TelemetryUtils() = default;

        static std::string getCloudPlatform();
        static std::string getMCSProxy();
    };
}