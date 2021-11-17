/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/HttpSender/IHttpSender.h>
#include <string>

namespace Telemetry
{
    class TelemetryUtils
    {
    public:
        TelemetryUtils() = default;

        static std::string getCloudPlatform();
    };
}