/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"

#include "SystemTelemetryCollectorImpl.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>

using namespace Telemetry;

void TelemetryProcessor::addTelemetry(const std::string& sourceName, const std::string& json)
{
    TelemetryHelper::getInstance().mergeJsonIn(sourceName, json);
}

void TelemetryProcessor::gatherTelemetry()
{
    LOGINFO("Gathering telemetry");

    // TODO: extract separate function for gathering system telemetry (LINUXEP-8094)
    SystemTelemetryCollectorImpl systemTelemetryCollector(
        GL_systemTelemetryObjectsConfig, GL_systemTelemetryArraysConfig);
    auto systemTelemetryObjects = systemTelemetryCollector.collectObjects();
    auto systemTelemetryArrays = systemTelemetryCollector.collectArraysOfObjects();

    for (const auto& [telemetryName, values] : systemTelemetryObjects)
    {
        if (values.size() == 1)
        {
            // just set a property and don't create a sub-object with just one property
            if (values[0].index() == 0)
            {
                TelemetryHelper::getInstance().set(telemetryName, std::get<std::string>(values[0]));
            }
            else
            {
                TelemetryHelper::getInstance().set(telemetryName, std::get<int>(values[0]));
            }
        }
        else
        {
            // TODO: need to create a sub-object
            for (const auto& value : values)
            {
                if (value.index() == 0)
                {
                    TelemetryHelper::getInstance().set(telemetryName, std::get<std::string>(value));
                }
                else
                {
                    TelemetryHelper::getInstance().set(telemetryName, std::get<int>(value));
                }
            }
        }
    }

    for (const auto& [telemetryName, arrays] : systemTelemetryArrays)
    {
        for (const auto& array : arrays)
        {
            // TODO: need to create a sub-object
            for (const auto& value : array)
            {
                if (value.index() == 0)
                {
                    TelemetryHelper::getInstance().append(telemetryName, std::get<std::string>(value));
                }
                else
                {
                    TelemetryHelper::getInstance().append(telemetryName, std::get<int>(value));
                }
            }
        }
    }

    std::cout << TelemetryHelper::getInstance().serialise(); // TODO remove!

    // TODO gather plugin telemetry LINUXEP-7972

    // This will gather telemetry from all the various sources and merge them in one at a time.
    std::string gatheredExampleJson = R"({"thing":2})";
    addTelemetry("TelemetryExecutableExample", gatheredExampleJson);
}

void TelemetryProcessor::saveTelemetryToDisk(const std::string& jsonOutputFile)
{
    LOGDEBUG("Saving telemetry to file: " << jsonOutputFile);

    // Will overwrite data each time.
    Common::FileSystem::fileSystem()->writeFile(jsonOutputFile, getSerialisedTelemetry());
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return TelemetryHelper::getInstance().serialise();
}

