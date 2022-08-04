/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "SusRequester.h"
#include "Logger.h"
#include "SDDS3Utils.h"

#include <utility>

SulDownloader::SDDS3::SusRequester::SusRequester(std::shared_ptr<Common::HttpRequests::IHttpRequester> httpClient) :
    m_httpClient(std::move(httpClient))
{
}

SulDownloader::SDDS3::SusResponse SulDownloader::SDDS3::SusRequester::request(const SUSRequestParameters& parameters)
{
    SusResponse susResponse;
    susResponse.success = false;
    try
    {
        std::string requestJson = SulDownloader::createSUSRequestBody(parameters);
        Common::HttpRequests::RequestConfig httpRequest;

        // URL
        httpRequest.url = parameters.baseUrl + "/v3/" + parameters.tenantId + "/" + parameters.deviceId;

        // Proxy
        if (parameters.proxy.empty())
        {
            LOGINFO("Trying SUS request (" << parameters.baseUrl << ") without proxy");
        }
        else
        {
            // Suldownloader allows implicit use of environment proxies and indicates that by setting the hostname of
            // the proxy to be suldownloaderdata::Proxy::EnvironmentProxy, so we need to tell the httprequest lib
            // to allow environment proxies to be automatically picked up by cURL.
            if (parameters.proxy.getUrl() == suldownloaderdata::Proxy::EnvironmentProxy)
            {
                httpRequest.allowEnvironmentProxy = true;
            }
            else
            {
                httpRequest.proxy = parameters.proxy.getUrl();
                LOGINFO("Trying SUS request (" << parameters.baseUrl << ") with proxy: " << httpRequest.proxy.value());
                if (!parameters.proxy.getCredentials().getUsername().empty() &&
                    !parameters.proxy.getCredentials().getPassword().empty())
                {
                    httpRequest.proxyUsername = parameters.proxy.getCredentials().getUsername();
                    httpRequest.proxyPassword = parameters.proxy.getCredentials().getDeobfuscatedPassword();
                }
            }
        }

        // Headers
        Common::HttpRequests::Headers headers;
        headers["Authorization"] = "Bearer " + parameters.jwt;
        headers["Content-Type"] = "application/json";
        headers["Content-Length"] = std::to_string(requestJson.length());
        httpRequest.headers = headers;

        // Timeout
        httpRequest.timeout = parameters.timeoutSeconds;

        // Body
        httpRequest.data = requestJson;

        // Perform request
        auto httpResponse = m_httpClient->post(httpRequest);

        if (httpResponse.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            LOGDEBUG("SUS request received HTTP response code: " << httpResponse.status);
            if (httpResponse.status == Common::HttpRequests::HTTP_STATUS_OK)
            {
                // Populate suites and releaseGroups
                LOGDEBUG("SUS response: " << httpResponse.body);
                parseSUSResponse(httpResponse.body, susResponse.data.suites, susResponse.data.releaseGroups);
                susResponse.success = true;
            }
            else
            {
                std::stringstream message;
                message << "SUS request received HTTP response code: " << httpResponse.status
                        << " but was expecting: " << Common::HttpRequests::HTTP_STATUS_OK;
                susResponse.error = message.str();
            }
        }
        else
        {
            std::stringstream message;
            message << "SUS request failed with error: " << httpResponse.error;
            susResponse.error = message.str();
        }
    }
    catch (const std::exception& ex)
    {
        std::stringstream message;
        message << "SUS request caused exception: " << ex.what();
        susResponse.error = message.str();
    }
    return susResponse;
}
