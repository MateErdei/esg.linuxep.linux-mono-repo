/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "HttpSender.h"

#include <Telemetry/LoggerImpl/Logger.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

#include <curl.h>
#include <map>
#include <sstream>

HttpSender::HttpSender(std::string server, std::shared_ptr<ICurlWrapper> curlWrapper) :
    m_server(std::move(server)),
    m_curlWrapper(std::move(curlWrapper))
{
    // Initialising ssl and crypto to create a dependency on their libraries and be able to get the correct rpath
    // [LINUXEP-6636] Rebuild libcurl with rpath set and remove these lines
    SSL_library_init();
    OPENSSL_init_crypto(OPENSSL_INIT_NO_ADD_ALL_CIPHERS, nullptr);
}

void HttpSender::setServer(const std::string& server)
{
    m_server = server;
}

std::string HttpSender::requestTypeToString(RequestType requestType)
{
    switch(requestType)
    {
        case RequestType::GET:
            return "GET";
        case RequestType::POST:
            return "POST";
        case RequestType::PUT:
            return "PUT";
        default:
            return "UNKNOWN";
    }
}

void HttpSender::setCurlOptions(
    CURL* curl,
    curl_slist* headers,
    const RequestType& requestType,
    const std::string& certPath,
    const std::vector<std::string>& additionalHeaders,
    const std::string& data)
{
    std::stringstream uriStream;
    uriStream << "https://" << m_server;
    std::string uri = uriStream.str();

    LOGINFO("Creating HTTPS " << requestTypeToString(requestType) << " Request to " << uri);

    CURLcode result;
    std::vector<std::tuple<const char*, CURLoption, const char*>> curlOptions;

    curlOptions.emplace_back("Specify network URL", CURLOPT_URL, uri.c_str());
    curlOptions.emplace_back("Specify path to Certificate Authority bundle", CURLOPT_CAINFO, certPath.c_str());

    for (const auto& header : additionalHeaders)
    {
        headers = m_curlWrapper->curlSlistAppend(headers, header.c_str());
        if (!headers)
        {
            throw std::runtime_error("Failed to append header to request");
        }
    }

    if (headers)
    {
        curlOptions.emplace_back("Specify custom HTTP header(s)", CURLOPT_HTTPHEADER, reinterpret_cast<const char*>(headers));
    }

    if (requestType == RequestType::POST)
    {
        curlOptions.emplace_back("Specify data to POST to server", CURLOPT_POSTFIELDS, data.c_str());
    }
    else if (requestType == RequestType::PUT)
    {
        curlOptions.emplace_back("Specify a custom PUT request", CURLOPT_CUSTOMREQUEST, "PUT");
        curlOptions.emplace_back("Specify data to POST to server", CURLOPT_POSTFIELDS, data.c_str());
    }

    for (const auto& curlOption : curlOptions)
    {
        result = m_curlWrapper->curlEasySetopt(curl, std::get<1>(curlOption), std::get<2>(curlOption));
        if (result != CURLE_OK)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to: " << std::get<0>(curlOption) << " with error: " << m_curlWrapper->curlEasyStrerror(result);
            throw std::runtime_error(errorMsg.str());
        }
    }
}

int HttpSender::httpsRequest(
    const RequestType& requestType,
    const std::string& certPath,
    const std::vector<std::string>& additionalHeaders,
    const std::string& data)
{
    CURL* curl = nullptr;
    CURLcode result = CURLE_FAILED_INIT;
    curl_slist* headers = nullptr;

    try
    {
        result = m_curlWrapper->curlGlobalInit(CURL_GLOBAL_DEFAULT); // NOLINT

        if (result != CURLE_OK)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to initialise libcurl with error: " << m_curlWrapper->curlEasyStrerror(result);
        }

        curl = m_curlWrapper->curlEasyInit();
        if (curl)
        {
            setCurlOptions(curl, headers, requestType, certPath, additionalHeaders, data);
            result = m_curlWrapper->curlEasyPerform(curl);

            if (result != CURLE_OK)
            {
                std::stringstream errorMsg;
                errorMsg << "Failed to perform file transfer with error: " << m_curlWrapper->curlEasyStrerror(result);
                throw std::runtime_error(errorMsg.str());
            }

            m_curlWrapper->curlSlistFreeAll(headers);
            m_curlWrapper->curlEasyCleanup(curl);
        }
        else
        {
            result = CURLE_FAILED_INIT;
        }

        m_curlWrapper->curlGlobalCleanup();
    }

    catch(const std::exception& e)
    {
        LOGERROR("Exception while making HTTPS " << requestTypeToString(requestType) << " request: " << e.what());
        m_curlWrapper->curlSlistFreeAll(headers);
        m_curlWrapper->curlEasyCleanup(curl);
        m_curlWrapper->curlGlobalCleanup();
    }

    return static_cast<int>(result);
}

int HttpSender::getRequest(const std::vector<std::string>& additionalHeaders, const std::string& certPath)
{
    return httpsRequest(RequestType::GET, certPath, additionalHeaders, "");
}

int HttpSender::postRequest(
    const std::vector<std::string>& additionalHeaders,
    const std::string& data,
    const std::string& certPath)
{
    return httpsRequest(RequestType::POST, certPath, additionalHeaders, data);
}

int HttpSender::putRequest(
    const std::vector<std::string>& additionalHeaders,
    const std::string& data,
    const std::string& certPath)
{
    return httpsRequest(RequestType::PUT, certPath, additionalHeaders, data);
}
