// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "TelemetryProcessor.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/HttpRequests/IHttpRequester.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>
#include <sys/stat.h>

#include <utility>

using namespace Telemetry;
using namespace Common::Telemetry;

TelemetryProcessor::TelemetryProcessor(
    std::shared_ptr<const Common::TelemetryConfigImpl::Config> config,
    std::shared_ptr<Common::HttpRequests::IHttpRequester> httpRequester,
    std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders) :
    m_config(std::move(config)), m_httpRequester(std::move(httpRequester)), m_telemetryProviders(std::move(telemetryProviders))
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
    Common::HttpRequests::RequestConfig requestConfig;
    requestConfig.url = "https://" + m_config->getServer() + "/" + m_config->getResourcePath();
    requestConfig.headers = m_config->getHeaders();
    requestConfig.port = m_config->getPort();
    requestConfig.timeout = 300;
    requestConfig.data = telemetryJson;

    if (!m_config->getTelemetryServerCertificatePath().empty() && Common::FileSystem::fileSystem()->isFile(m_config->getTelemetryServerCertificatePath()))
    {
        requestConfig.certPath = m_config->getTelemetryServerCertificatePath();
    }

    LOGINFO("Sending telemetry...");

    Common::HttpRequests::Response response;
    if (m_config->getVerb() == "GET")
    {
        response = m_httpRequester->get(requestConfig);
    }
    else if (m_config->getVerb() == "POST")
    {
        response = m_httpRequester->post(requestConfig);
    }
    else
    {
        response = m_httpRequester->put(requestConfig);
    }

    if (response.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
    {
        LOGDEBUG("HTTP result: " << response.status);

        if (response.status != Common::HttpRequests::HTTP_STATUS_OK)
        {
            std::stringstream msg;
            msg << "HTTP was expected to be 200, actual: " << response.status;
            throw std::runtime_error(msg.str());
        }

        LOGINFO("Sent telemetry");
    }
    else
    {
        LOGERROR("Failed to contact telemetry server (" << response.errorCode << "): " << response.error);
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
