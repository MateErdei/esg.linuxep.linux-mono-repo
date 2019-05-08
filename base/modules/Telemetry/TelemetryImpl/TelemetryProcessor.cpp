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


TelemetryProcessor::TelemetryProcessor(std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders, size_t maxJsonBytes) :
    m_telemetryProviders(std::move(telemetryProviders)), m_maxJsonSizeBytes(maxJsonBytes)
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
            LOGDEBUG("Could not get telemetry from one of the telemetry providers.");
        }
    }
}

void TelemetryProcessor::saveAndSendTelemetry()
{
    Path jsonOutputFile = Common::ApplicationConfiguration::applicationPathManager().getTelemetryOutputFilePath();
    LOGDEBUG("Saving telemetry to file: " << jsonOutputFile);

    std::string json = getSerialisedTelemetry();

    if (json.length() > m_maxJsonSizeBytes)
    {
        LOGERROR("The gathered telemetry exceeds the maximum size of " << m_maxJsonSizeBytes << " bytes.");
        return;
    }

    // TODO send telem LINUXEP-6637

    // Will overwrite data each time.
    Common::FileSystem::fileSystem()->writeFile(jsonOutputFile, json);
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return m_telemetryHelper.serialise();
}
