/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>

#include <sys/stat.h>
#include <utility>

using namespace Telemetry;
using namespace Common::Telemetry;

TelemetryProcessor::TelemetryProcessor(
    std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders,
    size_t maxJsonBytes) :
    m_telemetryProviders(std::move(telemetryProviders)),
    m_maxJsonSizeBytes(maxJsonBytes)
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

    if (json.length() > m_maxJsonSizeBytes)
    {
        std::stringstream msg;
        msg << "The gathered telemetry exceeds the maximum size of " << m_maxJsonSizeBytes << " bytes.";
        throw std::invalid_argument(msg.str());
    }

    // TODO: LINUXEP-7991 move sending here

    // Will overwrite data each time.
    Common::FileSystem::fileSystem()->writeFile(jsonOutputFile, json);
    Common::FileSystem::filePermissions()->chmod(jsonOutputFile, S_IRUSR | S_IWUSR);
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return m_telemetryHelper.serialise();
}
