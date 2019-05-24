/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryScheduler.h"

#include <Common/Logging/FileLoggingSetup.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <stdexcept>

namespace TelemetrySchedulerImpl
{
    int main_entry()
    {
        Common::Logging::FileLoggingSetup loggerSetup("telemetryScheduler", true);

        try
        {
            LOGINFO("Telemetry Scheduler running...");
        }
        catch (const std::runtime_error& e)
        {
            LOGERROR("Caught exception: " << e.what());
            return 1;
        }

        return 0;
    }

} // namespace TelemetryScheduler
