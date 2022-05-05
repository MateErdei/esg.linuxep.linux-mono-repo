/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#pragma once
#include <map>
#include <memory>
#include <optional>
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

        // used for dumping request contents for test
        friend std::ostream& operator<< (std::ostream& os, const RequestConfig& config)
        {
            os << "url: " + config.url + "\n";
            if(config.headers)
            {
                os << std::string("headers:\n");
                for(auto header : config.headers.value())
                {
                    os << header.first + ":" + header.second + "\n";
                }
            }
            if (config.data)
            {
                os << config.data.value() + "\n";
            }
            if (config.port)
            {
                os << std::to_string(config.port.value()) + "\n";
            }
            if(config.parameters)
            {
                os << std::string("parameters:\n");
                for(auto param : config.parameters.value())
                {
                    os << param.first + ":" + param.second + "\n";
                }
            }
            if (config.certPath)
            {
                os << config.certPath.value() + "\n";
            }
            if (config.fileDownloadLocation)
            {
                os << config.fileDownloadLocation.value() + "\n";
            }
            if (config.fileToUpload)
            {
                os << config.fileToUpload.value() + "\n";
            }
            if (config.proxy)
            {
                os << config.proxy.value() + "\n";
            }
            if (config.proxyUsername)
            {
                os << config.proxyUsername.value() + "\n";
            }
            if (config.proxyPassword)
            {
                os << config.proxyPassword.value() + "\n";
            }
            if (config.bandwidthLimit)
            {
                os << std::to_string(config.bandwidthLimit.value()) + "\n";
            }
            os << std::to_string(config.timeout) + "\n";
            os << std::to_string(config.allowRedirects) + "\n";
            os << std::to_string(config.requestType) + "\n";
            return os;
        }

    };
    inline bool operator==(const RequestConfig lhs,const RequestConfig rhs)
    {
        if (lhs.url != rhs.url){return false;}
        if (lhs.headers != rhs.headers){return false;}
        if (lhs.data != rhs.data){return false;}
        if (lhs.port != rhs.port){return false;}
        if (lhs.parameters != rhs.parameters){return false;}
        if (lhs.certPath != rhs.certPath){return false;}
        if (lhs.fileDownloadLocation != rhs.fileDownloadLocation){return false;}
        if (lhs.fileToUpload != rhs.fileToUpload){return false;}
        if (lhs.proxy != rhs.proxy){return false;}
        if (lhs.proxyUsername != rhs.proxyUsername){return false;}
        if (lhs.proxyPassword != rhs.proxyPassword){return false;}
        if (lhs.bandwidthLimit != rhs.bandwidthLimit){return false;}
        if (lhs.timeout != rhs.timeout){return false;}
        if (lhs.allowRedirects != rhs.allowRedirects){return false;}
        if (lhs.requestType != rhs.requestType){return false;}

        return true;
    }
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
