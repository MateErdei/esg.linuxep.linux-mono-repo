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
    Common::Telemetry::TelemetryHelper::getInstance().mergeJsonIn(sourceName, json);
}

void TelemetryProcessor::gatherTelemetry()
{
    LOGINFO("Gathering telemetry");

    auto systemTelemetryJson = gatherSystemTelemetry();
    Common::Telemetry::TelemetryHelper::getInstance().mergeJsonIn("system-telemetry", systemTelemetryJson);

    // TODO gather plugin telemetry LINUXEP-7972
}

std::string TelemetryProcessor::gatherSystemTelemetry()
{
    SystemTelemetryCollectorImpl systemTelemetryCollector(
        GL_systemTelemetryObjectsConfig, GL_systemTelemetryArraysConfig);
    auto systemTelemetryObjects = systemTelemetryCollector.collectObjects();
    auto systemTelemetryArrays = systemTelemetryCollector.collectArraysOfObjects();

    // TODO: create a top-level "system-telemetry" object

    Common::Telemetry::TelemetryHelper jsonConverter;

    for (const auto& [telemetryName, objects] : systemTelemetryObjects)
    {
        if (objects.size() == 1 && objects[0].first.empty())
        {
            // Just one unnamed value, so add as value rather than sub-object.
            if (objects[0].second.index() == 0)
            {
                jsonConverter.set(telemetryName, std::get<std::string>(objects[0].second));
            }
            else
            {
                jsonConverter.set(telemetryName, std::get<int>(objects[0].second));
            }

            continue;
        }

        // For now, no top-level objects are expected only values (handled above) and arrays of objects (handled below).
        throw std::logic_error("No top-level objects are expected for the current implementation.");
    }

    for (const auto& [telemetryName, array] : systemTelemetryArrays)
    {
        for (const auto& object : array)
        {
            Common::Telemetry::TelemetryObject& jsonObject = jsonConverter.appendObject(telemetryName);

            for (const auto& value : object)
            {
                Common::Telemetry::TelemetryValue jsonValue;

                if (value.second.index() == 0)
                {
                    jsonValue.set(std::get<std::string>(value.second));
                }
                else
                {
                    jsonValue.set(std::get<int>(value.second));
                }

                jsonObject.set(value.first, jsonValue);
            }
        }
    }

    return jsonConverter.serialise();
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

