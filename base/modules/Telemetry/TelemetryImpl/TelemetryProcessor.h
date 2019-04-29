/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <Common/TelemetryHelperImpl/TelemetryObject.h>
#include <utility>
#include "../../../thirdparty/nlohmann-json/json.hpp"
#include <Common/FileSystem/IFileSystem.h>

class TelemetryProcessor {
public:
    explicit TelemetryProcessor(std::string  jsonOutputPath);

    void run();

private:
    Path m_telemetryJsonOutput;

    void addTelemetry(const std::string& sourceName, const std::string& json);
    void gatherTelemetry();
    void sendTelemetry();
    void saveTelemetryToDisk(const std::string& jsonOutputFile);
    std::string getSerialisedTelemetry();

};

