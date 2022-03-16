/*
 * Copyright 2022, Sophos Limited.  All rights reserved.
 */

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace Common::HttpRequests
{
    enum RequestType
    {
        GET,
        POST,
        PUT,
        DELETE,
        OPTIONS
    };
    using Headers = std::map<std::string, std::string>;
    using Parameters = std::map<std::string, std::string>;

    struct RequestConfig
    {
        std::string url;
        std::optional<Common::HttpRequests::Headers> headers = std::nullopt;
        std::optional<std::string> data = std::nullopt;
        std::optional<unsigned int> port = std::nullopt;
        std::optional<Common::HttpRequests::Parameters> parameters = std::nullopt;
        std::optional<std::string> certPath = std::nullopt;
        std::optional<std::string> fileDownloadLocation = std::nullopt;
        std::optional<std::string> fileToUpload = std::nullopt;
        std::optional<std::string> proxy = std::nullopt;
        std::optional<std::string> proxyUsername = std::nullopt;
        std::optional<std::string> proxyPassword = std::nullopt;
        std::optional<unsigned int> bandwidthLimit = std::nullopt;
        unsigned long timeout = 600;
        bool allowRedirects = false;
        RequestType requestType = RequestType::GET;
    };

    const int STATUS_NOT_SET = -1;
    const int STATUS_OK = 200;
    struct Response
    {
        std::string body;
        std::string error;
        // TODO add main failure cases to interface as enum, e.g. convert these:CURLE_OPERATION_TIMEDOUT to non curl ones.
//        std::string errorCode;
        Headers headers;
        long status = STATUS_NOT_SET;
    };

    class HttpRequestsException : public std::exception
    {
    public:
        HttpRequestsException(const std::string& message) noexcept :
            m_message(message)
        {
        }
        ~HttpRequestsException() override = default;
        virtual const char* what() const noexcept override
        {
            return m_message.c_str();
        }
    private:
        std::string m_message;
    };


    class IHttpRequester
    {
    public:
        virtual ~IHttpRequester() = default;

       virtual Response get(RequestConfig) = 0;

       virtual Response post(RequestConfig) = 0;

       virtual Response put(RequestConfig) = 0;

       virtual Response del(RequestConfig) = 0;

       virtual Response options(RequestConfig) = 0;
    };
}
