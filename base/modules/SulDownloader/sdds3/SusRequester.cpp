// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SusRequester.h"

#include "ISignatureVerifierWrapper.h"
#include "Jwt.h"
#include "JwtVerificationException.h"
#include "Logger.h"
#include "SDDS3Utils.h"
#include "SusResponseParseException.h"
#include "SusResponseVerificationException.h"

#include "Common/SslImpl/Digest.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <nlohmann/json.hpp>
#include <utility>

SulDownloader::SDDS3::SusRequester::SusRequester(
    std::shared_ptr<Common::HttpRequests::IHttpRequester> httpClient,
    std::shared_ptr<ISignatureVerifierWrapper> verifier) :
    m_httpClient(std::move(httpClient)), verifier_(std::move(verifier))
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
            // the proxy to be Common::Policy::EnvironmentProxy, so we need to tell the httprequest lib
            // to allow environment proxies to be automatically picked up by cURL.
            if (parameters.proxy.getUrl() == Common::Policy::EnvironmentProxy)
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

        httpRequest.maxSize = MAX_SUS_SIZE;

        // Perform request
        auto httpResponse = m_httpClient->post(httpRequest);

        if (httpResponse.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            LOGDEBUG("SUS request received HTTP response code: " << httpResponse.status);
            if (httpResponse.status == Common::HttpRequests::HTTP_STATUS_OK)
            {
                // Populate suites and releaseGroups
                LOGDEBUG("SUS response: " << httpResponse.body);
                verifySUSResponse(*verifier_, httpResponse);
                parseSUSResponse(httpResponse.body, susResponse.data);
                if (susResponse.data.code.empty())
                {
                    susResponse.success = true;
                }
                else
                {
                    std::stringstream message;
                    message << "error: '" << susResponse.data.error << "' reason: '" << susResponse.data.reason
                            << "' code: '" << susResponse.data.code << "'";
                    susResponse.error = message.str();
                    std::vector<std::string> PERSISTENT_ERROR_CODES{
                        "FIXED_VERSION_TOKEN_NOT_FOUND",
                        "FIXED_VERSION_TOKEN_EOL",
                    };
                    if (std::find(
                            PERSISTENT_ERROR_CODES.begin(), PERSISTENT_ERROR_CODES.end(), susResponse.data.code) !=
                        PERSISTENT_ERROR_CODES.end())
                    {
                        susResponse.persistentError = true;
                    }
                }
            }
            else
            {
                std::stringstream message;
                message << "SUS request received HTTP response code: " << httpResponse.status
                        << " but was expecting: " << Common::HttpRequests::HTTP_STATUS_OK;
                susResponse.error = message.str();
                if (httpResponse.status < 500)
                {
                    susResponse.persistentError = true;
                }
            }
        }
        else if (
            httpResponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT ||
            httpResponse.errorCode == Common::HttpRequests::ResponseErrorCode::COULD_NOT_RESOLVE_HOST ||
            httpResponse.errorCode == Common::HttpRequests::ResponseErrorCode::COULD_NOT_RESOLVE_PROXY)
        {
            std::stringstream message;
            message << "SUS request failed to connect to the server with error: " << httpResponse.error;
            susResponse.error = message.str();
        }
        else
        {
            std::stringstream message;
            message << "SUS request failed with error: " << httpResponse.error;
            susResponse.error = message.str();
            susResponse.persistentError = true;
        }
    }
    // Shouldn't use .what_with_location() here as these errors are sent to Central
    catch (const JwtVerificationException& ex)
    {
        std::stringstream message;
        message << "Failed to verify JWT in SUS response: " << ex.what();
        susResponse.error = message.str();
        susResponse.persistentError = true;
    }
    catch (const SusResponseVerificationException& ex)
    {
        std::stringstream message;
        message << "Failed to verify SUS response: " << ex.what();
        susResponse.error = message.str();
        susResponse.persistentError = true;
    }
    catch (const SusResponseParseException& ex)
    {
        std::stringstream message;
        message << "Failed to parse SUS response: " << ex.what();
        susResponse.error = message.str();
        susResponse.persistentError = true;
    }
    catch (const std::exception& ex)
    {
        std::stringstream message;
        message << "SUS request caused exception: " << ex.what();
        susResponse.error = message.str();
        susResponse.persistentError = true;
    }
    return susResponse;
}

// Based on
// esg.em.esg/products/windows_endpoint/protection/sau/SDDSDownloader/SDDS3DownloaderLogic.cpp:verify_service_response
void SulDownloader::SDDS3::SusRequester::verifySUSResponse(
    const ISignatureVerifierWrapper& verifier,
    const Common::HttpRequests::Response& response)
{
    bool hasFound = false;
    std::string xContentSignature;
    for (const auto& [key, value] : response.headers)
    {
        // Headers are case-insensitive, so handle that
        // Real SUS sends the header as 'x-content-signature'
        std::string nameCopy = key; // Need to copy the key as toLower does the conversion in-place
        if (Common::UtilityImpl::StringUtils::toLower(nameCopy) == "x-content-signature")
        {
            xContentSignature = response.headers.at(key);
            hasFound = true;
        }
    }
    if (!hasFound || xContentSignature.empty())
    {
        throw SusResponseVerificationException(LOCATION, "SUS response does not have X-Content-Signature");
    }

    const auto payload = verifyAndLoadJwtPayload(verifier, xContentSignature);

    const auto& body = response.body;

    // Enforce 'size'
    if (!payload.contains("size") || payload["size"].type() != nlohmann::json::value_t::number_unsigned ||
        payload["size"] != body.size())
    {
        throw SusResponseVerificationException(
            LOCATION, "Malformed SUS response: size mismatch (actual size: " + std::to_string(body.size()) + ")");
    }

    // Enforce 'sha256'
    auto responseSha256 = Common::SslImpl::calculateDigest(Common::SslImpl::Digest::sha256, body);
    if (!payload.contains("sha256") || payload["sha256"].type() != nlohmann::json::value_t::string ||
        payload["sha256"] != responseSha256)
    {
        throw SusResponseVerificationException(
            "Malformed SUS response: sha256 mismatch (actual hash: " + responseSha256 + ")");
    }

    // TODO LINUXDAR-7772: timestamp verification
}

void SulDownloader::SDDS3::SusRequester::parseSUSResponse(const std::string& response, SusData& data)
{
    try
    {
        LOGDEBUG("Sus response: " << response);
        auto json = nlohmann::json::parse(response);

        if (json.contains("suites"))
        {
            for (const auto& s : json["suites"].items())
            {
                data.suites.insert(std::string(s.value()));
            }
        }

        if (json.contains("release-groups"))
        {
            for (const auto& g : json["release-groups"].items())
            {
                data.releaseGroups.insert(std::string(g.value()));
            }
        }

        if (json.contains("code"))
        {
            data.code = json["code"];
        }

        if (json.contains("error"))
        {
            data.error = json["error"];
        }

        if (json.contains("reason"))
        {
            data.reason = json["reason"];
        }
    }
    catch (const nlohmann::json::exception& exception)
    {
        std::stringstream errorMessage;
        errorMessage << "Failed to parse SUS response with error: " << exception.what()
                     << ", JSON content: " << response;
        std::throw_with_nested(SusResponseParseException(LOCATION, errorMessage.str()));
    }
}
