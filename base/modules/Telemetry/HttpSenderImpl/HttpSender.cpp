/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "HttpSender.h"

#include <Telemetry/LoggerImpl/Logger.h>

#include <curl.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <sstream>

HttpSender::HttpSender(
        std::string server,
        int port,
        std::shared_ptr<ICurlWrapper> curlWrapper)
: m_server(std::move(server))
, m_port(port)
, m_curlWrapper(std::move(curlWrapper))
{
    // Initialising ssl and crypto to create a dependency on their libraries and be able to get the correct rpath
    // TODO: Rebuild libcurl with rpath set and remove these lines
    SSL_library_init();
    OPENSSL_init_crypto(OPENSSL_INIT_NO_ADD_ALL_CIPHERS, nullptr);
}

void HttpSender::setServer(const std::string& server)
{
    m_server = server;
}

void HttpSender::setPort(const int& port)
{
    m_port = port;
}

int HttpSender::httpsRequest(
        const std::string& verb,
        const std::string& certPath,
        const std::vector<std::string>& additionalHeaders,
        const std::string& jsonStruct)
{
    std::stringstream uri;
    uri << "https://" << m_server << ":" << m_port;

    std::stringstream msg;
    msg << "Creating HTTP " << verb << " Request to " << uri.str();
    LOGINFO(msg.str());

    CURL *curl;
    CURLcode result = CURLE_FAILED_INIT;

    m_curlWrapper->curlGlobalInit(CURL_GLOBAL_DEFAULT); // NOLINT

    curl = m_curlWrapper->curlEasyInit();
    if(curl)
    {
        m_curlWrapper->curlEasySetopt(curl, CURLOPT_URL, uri.str().c_str());
        m_curlWrapper->curlEasySetopt(curl, CURLOPT_CAINFO, certPath.c_str()); // TODO: Set to "/opt/sophos-spl/base/etc"

        struct curl_slist *headers = nullptr;
        headers = m_curlWrapper->curlSlistAppend(headers, "Accept: application/json");
        headers = m_curlWrapper->curlSlistAppend(headers, "Content-Type: application/json");
        headers = m_curlWrapper->curlSlistAppend(headers, "charsets: utf-8");
        for (auto const& header : additionalHeaders)
        {
            headers = m_curlWrapper->curlSlistAppend(headers, header.c_str());
        }

        m_curlWrapper->curlEasySetopt(curl, CURLOPT_HTTPHEADER, reinterpret_cast<const char*>(headers));

        if(verb == "POST")
        {
            m_curlWrapper->curlEasySetopt(curl, CURLOPT_POSTFIELDS, jsonStruct.c_str());
        }

        result = m_curlWrapper->curlEasyPerform(curl);

        if(result != CURLE_OK)
        {
            std::stringstream errMsg;
            errMsg << "curl_easy_perform() failed: " << m_curlWrapper->curlEasyStrerror(result);
            LOGERROR(errMsg.str());
        }

        m_curlWrapper->curlSlistFreeAll(headers);
        m_curlWrapper->curlEasyCleanup(curl);
    }

    m_curlWrapper->curlGlobalCleanup();

    return static_cast<int>(result);
}

int HttpSender::getRequest(
        const std::vector<std::string>& additionalHeaders,
        const std::string& certPath)
{
    return httpsRequest("GET", certPath, additionalHeaders, "");
}

int HttpSender::postRequest(
        const std::vector<std::string> &additionalHeaders,
        const std::string &jsonStruct,
        const std::string& certPath)
{
    return httpsRequest("POST", certPath, additionalHeaders, jsonStruct);
}