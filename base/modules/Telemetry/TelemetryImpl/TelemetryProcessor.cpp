/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/HttpSender/IHttpSender.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>
#include <sys/stat.h>

#include <curl.h>
#include <utility>

using namespace Telemetry;
using namespace Common::Telemetry;

TelemetryProcessor::TelemetryProcessor(
    std::shared_ptr<const Common::TelemetryConfigImpl::Config> config,
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

    LOGDEBUG("Gathered telemetry: " << telemetryJson);

    try
    {
        saveTelemetry(telemetryJson);
    }
    catch (Common::FileSystem::IFileSystemException& ex)
    {
        LOGERROR("Could not save telemetry JSON to disk: " << ex.what());
    }

    sendTelemetry(telemetryJson);
}

void TelemetryProcessor::addTelemetry(const std::string& sourceName, const std::string& json)
{
    if (json.length() > m_config->getMaxJsonSize())
    {
        std::stringstream msg;
        msg << "The telemetry being added exceeds the maximum size of " << m_config->getMaxJsonSize() << " bytes.";
        throw std::runtime_error(msg.str());
    }

    m_telemetryHelper.mergeJsonIn(sourceName, json);
}

void TelemetryProcessor::gatherTelemetry()
{
    LOGINFO("Gathering telemetry");

    for (const auto& provider : m_telemetryProviders)
    {
        std::string name = provider->getName();

        try
        {
            std::string telemetry = provider->getTelemetry();
            LOGINFO("Gathered telemetry for " << name);
            LOGDEBUG("Telemetry data gathered: " << telemetry);
            addTelemetry(name, telemetry);
        }
        catch (std::exception& ex)
        {
            LOGWARN("Could not get telemetry for " << name << ". Exception: " << ex.what());
        }
    }
}

void TelemetryProcessor::sendTelemetry(const std::string& telemetryJson)
{
    Common::HttpSender::RequestConfig requestConfig(
        Common::HttpSender::RequestConfig::stringToRequestType(m_config->getVerb()),
        m_config->getHeaders(),
        m_config->getServer(),
        m_config->getPort(),
        m_config->getTelemetryServerCertificatePath(),
        m_config->getResourcePath());

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

    LOGINFO("Sent telemetry");
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