/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>

using namespace Telemetry;
using namespace Common::Telemetry;

TelemetryProcessor::TelemetryProcessor(std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders) :
    m_telemetryProviders(std::move(telemetryProviders))
{
}

void TelemetryProcessor::addTelemetry(const std::string& sourceName, const std::string& json)
{
    m_telemetryHelper.mergeJsonIn(sourceName, json);
}

void TelemetryProcessor::gatherTelemetry()
{
    LOGINFO("Gathering telemetry");
    for (const std::shared_ptr<ITelemetryProvider>& provider : m_telemetryProviders)
    {
        addTelemetry(provider->getName(), provider->getTelemetry());
    }

    // TODO: move code below
    //    SystemTelemetryCollectorImpl collector(GL_systemTelemetryObjectsConfig, GL_systemTelemetryArraysConfig);
    //    SystemTelemetryReporter systemTelemetryReporter(collector);
    //    auto systemTelemetryJson = systemTelemetryReporter.gatherSystemTelemetry();
    //
    //    Common::Telemetry::TelemetryHelper::getInstance().mergeJsonIn("system-telemetry", systemTelemetryJson);
}

void TelemetryProcessor::saveTelemetryToDisk(const std::string& jsonOutputFile)
{
    LOGDEBUG("Saving telemetry to file: " << jsonOutputFile);

    // Will overwrite data each time.
    Common::FileSystem::fileSystem()->writeFile(jsonOutputFile, getSerialisedTelemetry());
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return m_telemetryHelper.serialise();
}
