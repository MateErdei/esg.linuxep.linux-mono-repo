/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "HttpSender.h"

#include <Telemetry/TelemetryImpl/Logger.h>

#include <curl.h>
#include <sstream>

HttpSender::HttpSender(
        std::string server,
        int port,
        std::shared_ptr<ICurlWrapper> curlWrapper)
: m_server(std::move(server))
, m_port(port)
, m_curlWrapper(curlWrapper)
{

}

void HttpSender::setServer(const std::string& server)
{
    m_server = server;
}

void HttpSender::setPort(const int& port)
{
    m_port = port;
}

void HttpSender::https_request(
        const std::string& verb,
        const std::vector<std::string>& additionalHeaders,
        const std::string& jsonStruct)
{
    std::stringstream uri;
    uri << "https://" << m_server << ":" << m_port;

    std::stringstream msg;
    msg << "Creating HTTP " << verb << " Request to " << uri.str();
    LOGINFO(msg.str());

    CURL *curl;
    CURLcode result;

    m_curlWrapper->curlGlobalInit(CURL_GLOBAL_DEFAULT); // NOLINT

    curl = m_curlWrapper->curlEasyInit();
    if(curl)
    {
        m_curlWrapper->curlEasySetopt(curl, CURLOPT_URL, uri.str().c_str());
        m_curlWrapper->curlEasySetopt(curl, CURLOPT_CAPATH, "/opt/sophos-spl/base/etc");

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
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    m_curlWrapper->curlEasyStrerror(result));

        m_curlWrapper->curlSlistFreeAll(headers);
        m_curlWrapper->curlEasyCleanup(curl);
    }

    m_curlWrapper->curlGlobalCleanup();
}

void HttpSender::get_request(
        const std::vector<std::string>& additionalHeaders)
{
    https_request("GET", additionalHeaders, "");
}

void HttpSender::post_request(
        const std::vector<std::string>& additionalHeaders,
        const std::string& jsonStruct)
{
    https_request("POST", additionalHeaders, jsonStruct);
}