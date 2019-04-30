/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <Common/TelemetryHelperImpl/TelemetryObject.h>
#include <utility>
#include "../../../thirdparty/nlohmann-json/json.hpp"
#include <Common/FileSystem/IFileSystem.h>

namespace Telemetry::TelemetryProcessor
{
    void addTelemetry(const std::string& sourceName, const std::string& json);
    void gatherTelemetry();
    void saveTelemetryToDisk(const std::string& jsonOutputFile);
    std::string getSerialisedTelemetry();
}