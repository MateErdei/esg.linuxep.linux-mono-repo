/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "HttpSender.h"
#include "Logger.h"

#include <curl.h>
#include <sstream>

HttpSender::HttpSender(
        std::string server,
        int port)
: m_server(std::move(server))
, m_port(port)
{

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
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT); // NOLINT

    curl = curl_easy_init();
    if(curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, uri.str().c_str());
        curl_easy_setopt(curl, CURLOPT_CAPATH, "/opt/sophos-spl/base/etc");

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");
        for (auto const& header : additionalHeaders)
        {
            headers = curl_slist_append(headers, header.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if(verb == "POST")
        {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStruct.c_str());
        }

        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
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