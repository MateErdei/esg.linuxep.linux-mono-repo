/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/HttpSender/IHttpSender.h>
#include <Common/OSUtilitiesImpl/SXLMachineID.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>
#include <sys/stat.h>

#include <curl.h>
#include <utility>

using namespace Telemetry;
using namespace Common::Telemetry;

TelemetryProcessor::TelemetryProcessor(
    std::shared_ptr<const Common::TelemetryExeConfigImpl::Config> config,
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

    if (telemetryJson.length() > m_config->getMaxJsonSize())
    {
        std::stringstream msg;
        msg << "The gathered telemetry exceeds the maximum size of " << m_config->getMaxJsonSize() << " bytes.";
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
    Common::OSUtilitiesImpl::SXLMachineID sxlMachineId;
    std::string machineId = sxlMachineId.getMachineID();

    // TODO: probably ought to get resource via a configuration setting and let the Telemetry Scheduler determine the
    // value to use!

    if (machineId.empty())
    {
        throw std::runtime_error("No machine id so not sending telemetry");
    }

    machineId.erase(
        std::remove_if(machineId.begin(), machineId.end(), [](auto const& c) -> bool { return !std::isalnum(c); }),
        machineId.end());

    std::string resourceRoot = m_config->getResourceRoot();

    if (resourceRoot.back() != '/')
    {
        resourceRoot.push_back('/');
    }

    std::string resourcePath = resourceRoot + machineId + ".json";

    Common::HttpSenderImpl::RequestConfig requestConfig(
        Common::HttpSenderImpl::RequestConfig::stringToRequestType(m_config->getVerb()),
        m_config->getHeaders(),
        m_config->getServer(),
        m_config->getPort(),
        m_config->getTelemetryServerCertificatePath(),
        resourcePath);

    if (!requestConfig.getCertPath().empty() && !Common::FileSystem::fileSystem()->isFile(requestConfig.getCertPath()))
    {
        throw std::runtime_error("Certificate file is not valid");
    }

    requestConfig.setData(telemetryJson);

    LOGINFO("Sending telemetry...");
    auto result = m_httpSender->doHttpsRequest(requestConfig);
    LOGDEBUG("HTTP request resulted in CURL result: " << result);

    if (result != CURLE_OK)
    {
        std::stringstream msg;
        msg << "HTTP request failed with CURL result " << result;
        throw std::runtime_error(msg.str());
    }
}

void TelemetryProcessor::saveTelemetry(const std::string& telemetryJson) const
{
    Path outputFile = Common::ApplicationConfiguration::applicationPathManager().getTelemetryOutputFilePath();
    LOGINFO("Saving telemetry to file: " << outputFile);
    Common::FileSystem::fileSystem()->writeFile(outputFile, telemetryJson); // Overwrites data each time.
    Common::FileSystem::filePermissions()->chmod(outputFile, S_IRUSR | S_IWUSR);
}

std::string TelemetryProcessor::getSerialisedTelemetry()
{
    return m_telemetryHelper.serialise();
}
