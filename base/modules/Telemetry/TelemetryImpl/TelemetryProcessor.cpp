/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"

#include "SystemTelemetryCollectorImpl.h"
#include "SystemTelemetryReporter.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>

using namespace Telemetry;

void TelemetryProcessor::addTelemetry(const std::string& sourceName, const std::string& json)
{
    Common::Telemetry::TelemetryHelper::getInstance().mergeJsonIn(sourceName, json);
}

void TelemetryProcessor::gatherTelemetry()
{
    LOGINFO("Gathering telemetry");

    SystemTelemetryCollectorImpl systemTelemetryCollector(
        GL_systemTelemetryObjectsConfig, GL_systemTelemetryArraysConfig);
    auto systemTelemetryJson = gatherSystemTelemetry(systemTelemetryCollector);
    Common::Telemetry::TelemetryHelper::getInstance().mergeJsonIn("system-telemetry", systemTelemetryJson);

    // TODO gather plugin telemetry LINUXEP-7972
}

void TelemetryProcessor::saveTelemetryToDisk(const std::string& jsonOutputFile)
{
    LOGDEBUG("Saving telemetry to file: " << jsonOutputFile);

    // Will overwrite data each time.
    Common::FileSystem::fileSystem()->writeFile(jsonOutputFile, getSerialisedTelemetry());
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return Common::Telemetry::TelemetryHelper::getInstance().serialise();
}

