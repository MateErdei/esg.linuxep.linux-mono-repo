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
    std::shared_ptr<const TelemetryConfig::Config> config,
    std::unique_ptr<Common::HttpSender::IHttpSender> httpSender,
    std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders) :
    m_config(config),
    m_httpSender(std::move(httpSender)),
    m_telemetryProviders(std::move(telemetryProviders))
{
}

void TelemetryProcessor::Run()
{
    gatherTelemetry();
    std::string telemetryJson = getSerialisedTelemetry();

    if (telemetryJson.length() > m_config->m_maxJsonSize)
    {
        std::stringstream msg;
        msg << "The gathered telemetry exceeds the maximum size of " << m_config->m_maxJsonSize << " bytes.";
        throw std::runtime_error(msg.str());
    }

    sendTelemetry(telemetryJson);
    saveTelemetry(telemetryJson);
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

void TelemetryProcessor::sendTelemetry(const std::string& telemetryJson)
{
    Common::HttpSenderImpl::RequestConfig requestConfig(m_config->m_verb, m_config->m_headers);
    requestConfig.setServer(m_config->m_server);
    requestConfig.setCertPath(m_config->m_certPath);
    requestConfig.setResourceRoot(m_config->m_resourceRoute);

    if (!Common::FileSystem::fileSystem()->isFile(requestConfig.getCertPath()))
    {
        throw std::runtime_error("Certificate file is not valid");
    }

    requestConfig.setData(telemetryJson);

    LOGINFO("Sending telemetry...");
    m_httpSender->doHttpsRequest(requestConfig);
}

void TelemetryProcessor::saveTelemetry(const std::string& telemetryJson) const
{
    Path outputFile = Common::ApplicationConfiguration::applicationPathManager().getTelemetryOutputFilePath();
    LOGINFO("Saving telemetry to file: " << outputFile);
    Common::FileSystem::fileSystem()->writeFile(outputFile, telemetryJson); // Overwrites data each time.
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return m_telemetryHelper.serialise();
}
