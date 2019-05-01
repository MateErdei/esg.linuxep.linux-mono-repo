/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ISystemTelemetryCollector.h"
#include "Logger.h"
#include "SystemTelemetryCollectorImpl.h"

#include <Common/Logging/FileLoggingSetup.h>

#include <sstream>

namespace Telemetry
{
    int main_entry(int argc, char* argv[])
    {
        // Configure logging
        Common::Logging::FileLoggingSetup loggerSetup("telemetry");

        std::stringstream msg;
        msg << "Running telemetry executable with arguments: ";
        for (int i = 0; i < argc; ++i)
        {
            msg << argv[i] << " ";
        }
        LOGINFO(msg.str());

        SystemTelemetryCollectorImpl systemTelemetryCollector(
            GL_systemTelemetryObjectsConfig, GL_systemTelemetryArraysConfig);
        auto systemTelemetryObjects = systemTelemetryCollector.collectObjects();
        auto systemTelemetryArrays = systemTelemetryCollector.collectArraysOfObjects();

        for (const auto& [telemetryItem, values] : systemTelemetryObjects)
        {
            for (const auto& value : values)
            {
                LOGDEBUG(
                    telemetryItem
                    << ": "
                    << ((value.index() == 0) ? std::get<std::string>(value) : std::to_string(std::get<int>(value))));
            }
        }

        // TODO: [LINUXEP-8094] append system telemetry to overall telemetry JSON document

        return 0;
    }
} // namespace Telemetry
