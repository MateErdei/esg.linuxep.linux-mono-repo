/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

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
        std::optional<long> port = std::nullopt;
        std::optional<Common::HttpRequests::Parameters> parameters = std::nullopt;
        std::optional<std::string> certPath = std::nullopt;
        std::optional<std::string> fileDownloadLocation = std::nullopt;
        std::optional<std::string> fileToUpload = std::nullopt;
        std::optional<std::string> proxy = std::nullopt;
        std::optional<std::string> proxyUsername = std::nullopt;
        std::optional<std::string> proxyPassword = std::nullopt;
        std::optional<long> bandwidthLimit = std::nullopt;
        long timeout = 600;
        bool allowRedirects = false;
        RequestType requestType = RequestType::GET;
    };

    enum ResponseErrorCode
    {
        // Request was performed
        OK,

        // Request timed out
        TIMEOUT,

        // If a user tries to download over the top of an existing file, this lib does not delete files
        DOWNLOAD_TARGET_ALREADY_EXISTS,

        // If a user tries to upload a nonexistent file
        UPLOAD_FILE_DOES_NOT_EXIST,

        // Problem with certs occurred, such as the certificate did not exist.
        CERTIFICATE_ERROR,

        // We got to the point of performing the request, however it failed
        REQUEST_FAILED,

        // General library failure
        FAILED
    };

    const int STATUS_NOT_SET = -1;

    struct Response
    {
        std::string body;
        std::string error;
        ResponseErrorCode errorCode = FAILED;
        Headers headers;
        long status = STATUS_NOT_SET;
    };

    class HttpRequestsException : public std::exception
    {
    public:
        HttpRequestsException(const std::string& message) noexcept : m_message(message) {}
        ~HttpRequestsException() override = default;
        virtual const char* what() const noexcept override { return m_message.c_str(); }

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
} // namespace Common::HttpRequests
