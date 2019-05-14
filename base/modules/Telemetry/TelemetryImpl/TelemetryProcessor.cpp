/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/HttpSender/IHttpSender.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>

#include <utility>

using namespace Telemetry;
using namespace Common::Telemetry;

TelemetryProcessor::TelemetryProcessor(
    const TelemetryConfig::Config& config,
    std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders) :
    m_config(config),
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

void TelemetryProcessor::saveAndSendTelemetry(Common::HttpSender::IHttpSender& httpSender)
{
    Path jsonOutputFile = Common::ApplicationConfiguration::applicationPathManager().getTelemetryOutputFilePath();

    std::string json = getSerialisedTelemetry();

    Common::HttpSenderImpl::RequestConfig requestConfig(m_config.m_verb, m_config.m_headers);
    requestConfig.setServer(m_config.m_server);
    requestConfig.setCertPath(m_config.m_certPath);
    requestConfig.setResourceRoot(m_config.m_resourceRoute);

    if (!Common::FileSystem::fileSystem()->isFile(requestConfig.getCertPath()))
    {
        throw std::runtime_error("Certificate is not a valid file");
    }

    requestConfig.setData(json);

    LOGINFO("Sending telemetry...");
    httpSender.doHttpsRequest(requestConfig);

    LOGINFO("Saving telemetry to file: " << jsonOutputFile);
    Common::FileSystem::fileSystem()->writeFile(jsonOutputFile, json); // Will overwrite data each time.
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return m_telemetryHelper.serialise();
}
