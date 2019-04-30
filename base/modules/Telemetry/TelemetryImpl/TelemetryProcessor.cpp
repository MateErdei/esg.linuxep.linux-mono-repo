/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"
#include <Telemetry/LoggerImpl/Logger.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>

using namespace Common::Telemetry;

TelemetryProcessor::TelemetryProcessor(std::string jsonOutputPath)
    : m_telemetryJsonOutput(std::move(jsonOutputPath))
{
}

void TelemetryProcessor::run()
{
    LOGINFO("Telemetry processing started");
    gatherTelemetry();
    saveTelemetryToDisk(m_telemetryJsonOutput);
    sendTelemetry();
}

void TelemetryProcessor::addTelemetry(const std::string& sourceName, const std::string& json)
{
    TelemetryHelper::getInstance().mergeJsonIn(sourceName, json);
}

void TelemetryProcessor::gatherTelemetry()
{
    LOGINFO("Gathering telemetry");

    // TODO gather plugin telemetry LINUXEP-7972
    // TODO gather system telemetry LINUXEP-8094

    // This will gather telemetry from all the various sources and merge them in one at a time.
    std::string gatheredExampleJson = R"({"thing":2})";
    addTelemetry("TelemetryExecutableExample", gatheredExampleJson);
}

void TelemetryProcessor::sendTelemetry()
{
    LOGINFO("Sending telemetry");
    LOGDEBUG("Sending telemetry: " << getSerialisedTelemetry());
}

void TelemetryProcessor::saveTelemetryToDisk(const std::string& jsonOutputFile)
{
    LOGDEBUG("Saving telemetry to file: " << jsonOutputFile);
    Common::FileSystem::fileSystem()->writeFile(jsonOutputFile, getSerialisedTelemetry());
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return TelemetryHelper::getInstance().serialise();
}
