/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerStatus.h"

namespace TelemetrySchedulerImpl
{
    class SchedulerStatusSerialiser
    {
    public:
        static std::string serialise(const SchedulerStatus& config);
        static SchedulerStatus deserialise(const std::string& jsonString);
    };

} // namespace TelemetrySchedulerImpl