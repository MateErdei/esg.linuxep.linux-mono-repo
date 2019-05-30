/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerConfig.h"

namespace TelemetrySchedulerImpl
{
    class Serialiser
    {
    public:
        static std::string serialise(const SchedulerConfig& config);
        static SchedulerConfig deserialise(const std::string& jsonString);
    };

} // namespace TelemetrySchedulerImpl