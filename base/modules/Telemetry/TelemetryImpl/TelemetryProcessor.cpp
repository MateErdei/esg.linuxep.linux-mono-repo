#include <utility>

/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
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
    for (const auto& provider : m_telemetryProviders)
    {
        try
        {
            addTelemetry(provider->getName(), provider->getTelemetry());
        }
        catch (std::exception& ex)
        {
            LOGWARN("Could not get telemetry from one of the telemetry providers. Exception: " << ex.what());
        }
    }
}

void TelemetryProcessor::saveAndSendTelemetry()
{
    Path jsonOutputFile = Common::ApplicationConfiguration::applicationPathManager().getTelemetryOutputFilePath();
    LOGINFO("Saving telemetry to file: " << jsonOutputFile);

    std::string json = getSerialisedTelemetry();

    // TODO Sending will be added in a later ticket, LINUXEP-7991

    // Will overwrite data each time.
    Common::FileSystem::fileSystem()->writeFile(jsonOutputFile, json);
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return m_telemetryHelper.serialise();
}
