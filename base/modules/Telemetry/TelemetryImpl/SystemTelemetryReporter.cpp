/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemTelemetryReporter.h"

#include "SystemTelemetryCollectorImpl.h"
#include "TelemetryProcessor.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>

namespace Telemetry
{
    SystemTelemetryReporter::SystemTelemetryReporter(
        std::unique_ptr<ISystemTelemetryCollector> systemTelemetryCollector) :
        m_systemTelemetryCollector(std::move(systemTelemetryCollector))
    {
        if (!m_systemTelemetryCollector)
        {
            throw std::invalid_argument("systemTelemetryCollector null");
        }
    }

    std::string SystemTelemetryReporter::getTelemetry()
    {
        Common::Telemetry::TelemetryHelper jsonConverter;

        auto systemTelemetryObjects = m_systemTelemetryCollector->collectObjects();
        getSimpleTelemetry(jsonConverter, systemTelemetryObjects);

        auto systemTelemetryArrays = m_systemTelemetryCollector->collectArraysOfObjects();
        getArraysTelemetry(jsonConverter, systemTelemetryArrays);

        return jsonConverter.serialise();
    }

    void SystemTelemetryReporter::getSimpleTelemetry(
        Common::Telemetry::TelemetryHelper& jsonConverter,
        const std::map<std::string, TelemetryItem>&
            systemTelemetryObjects)
    {
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
                    jsonConverter.set(telemetryName, (long)std::get<int>(objects[0].second));
                }

                continue;
            }

            // For now, no top-level objects are expected, only values (handled above) and arrays of objects (handled
            // below).
            throw std::logic_error("No top-level objects are expected for the current implementation.");
        }
    }

    void SystemTelemetryReporter::getArraysTelemetry(
        Common::Telemetry::TelemetryHelper& jsonConverter,
        const std::map<std::string, std::vector<TelemetryItem>>&
            systemTelemetryArrays)
    {
        for (const auto& [telemetryName, array] : systemTelemetryArrays)
        {
            for (const auto& object : array)
            {
                Common::Telemetry::TelemetryObject jsonObject;

                for (const auto& value : object)
                {
                    Common::Telemetry::TelemetryValue jsonValue;

                    if (value.second.index() == 0)
                    {
                        jsonValue.set(std::get<std::string>(value.second));
                    }
                    else
                    {
                        jsonValue.set((long)std::get<int>(value.second));
                    }

                    jsonObject.set(value.first, jsonValue);
                }

                jsonConverter.appendObject(telemetryName, jsonObject);
            }
        }
    }
}