// Copyright 2022-2024 Sophos Limited. All rights reserved.

#pragma once
#include "Common/CurlWrapper/ICurlWrapper.h"
#include "Common/HttpRequests/IHttpRequester.h"


namespace Common::HttpRequestsImpl
{

    struct ResponseBuffer
    {
        // This struct is used both in header and file write functions.
        // To allow users the option to specify just the download dir (not the name of the file to save to)
        // and for us to handle the case where a URL does not have the filename in explicitly,
        // then we need to work out the filename from the headers, if not fall back to using the url.

        // Used in header write func to store headers before putting them in the response obj and allows
        // the curl file write function access to the headers to work out the file names.
        HttpRequests::Headers headers;

        // URL also needs to be accessed by curl file write if headers don't contain a content disposition option.
        std::string url;

        // Directory to download the file to, if no file name explicitly set.
        std::string downloadDirectory;

        // File will be saved to this path.
        // This value is either by a user or by either header or URL parsing.
        std::string downloadFilePath;
    };

    class HttpRequesterImpl : public Common::HttpRequests::IHttpRequester
    {
    public:
        HttpRequesterImpl(std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper);
        HttpRequesterImpl(const HttpRequesterImpl&) = delete;
        HttpRequesterImpl& operator=(const HttpRequesterImpl&) = delete;
        ~HttpRequesterImpl();
        Common::HttpRequests::Response get(Common::HttpRequests::RequestConfig request) override;
        Common::HttpRequests::Response post(Common::HttpRequests::RequestConfig request) override;
        Common::HttpRequests::Response put(Common::HttpRequests::RequestConfig request) override;
        Common::HttpRequests::Response del(Common::HttpRequests::RequestConfig request) override;
        Common::HttpRequests::Response options(Common::HttpRequests::RequestConfig request) override;
        void sendTerminate() override;
    private:
        Common::HttpRequests::Response performRequest(Common::HttpRequests::RequestConfig config);
        std::string curlEscape(const std::string& stringToEscape);
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> m_curlWrapper;
        CURL* m_curlHandle;
        bool m_curlVerboseLogging = false;
    };

} // namespace Common::HttpRequestsImpl
