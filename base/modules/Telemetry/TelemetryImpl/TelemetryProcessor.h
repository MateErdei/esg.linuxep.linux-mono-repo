/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include "json.hpp"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelper/ITelemetryHelper.h>

#include <utility>

namespace Telemetry::TelemetryProcessor
{

    void addTelemetry(const std::string& sourceName, const std::string& json);
    void gatherTelemetry();
    void saveTelemetryToDisk(const std::string& jsonOutputFile);
    // TODO void sendTelemetry();

    std::string getSerialisedTelemetry();
    std::string gatherSystemTelemetry();
}