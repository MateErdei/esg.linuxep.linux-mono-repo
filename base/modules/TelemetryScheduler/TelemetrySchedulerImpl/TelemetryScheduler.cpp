/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryScheduler.h"

#include "SchedulerPluginCallback.h"

#include <Common/Logging/FileLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <stdexcept>

namespace TelemetrySchedulerImpl
{
    int main_entry()
    {
        try
        {
            Common::Logging::FileLoggingSetup loggerSetup("telemetryScheduler", true);

            LOGINFO("Telemetry Scheduler running...");

            std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
                Common::PluginApi::createPluginResourceManagement();

            auto sharedPluginCallBack = std::make_shared<SchedulerPluginCallback>();
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;

            try
            {
                // baseService = resourceManagement->createPluginAPI("tscheduler", sharedPluginCallBack);
            }
            catch (const Common::PluginApi::ApiException& apiException)
            {
                LOGERROR(apiException.what());
                throw;
            }

            LOGINFO("Telemetry Scheduler finished");
        }
        catch (const std::runtime_error& e)
        {
            LOGERROR("Caught exception: " << e.what());
            return 1;
        }

        return 0;
    }

} // namespace TelemetrySchedulerImpl
