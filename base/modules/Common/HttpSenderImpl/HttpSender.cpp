/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "HttpSender.h"
#include "Logger.h"

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <curl.h>
#include <map>
#include <sstream>

namespace
{
    class CurlScopeGuard
    {
        Common::HttpSender::ICurlWrapper& m_iCurlWrapper;
        CURL* m_curl;

    public:
        CurlScopeGuard(CURL* curl, Common::HttpSender::ICurlWrapper& iCurlWrapper)
        : m_iCurlWrapper(iCurlWrapper)
        , m_curl(curl)
        {

        }

        ~CurlScopeGuard()
        {
            m_iCurlWrapper.curlEasyCleanup(m_curl);
        }
    };

    class SListScopeGuard
    {
        Common::HttpSender::ICurlWrapper& m_iCurlWrapper;
        curl_slist* m_curl_slist;

    public:
        SListScopeGuard(curl_slist* curl_slist, Common::HttpSender::ICurlWrapper& iCurlWrapper)
        : m_iCurlWrapper(iCurlWrapper)
        , m_curl_slist (curl_slist)
        {

        }

        ~SListScopeGuard()
        {
            if( m_curl_slist != nullptr)
            {
                m_iCurlWrapper.curlSlistFreeAll(m_curl_slist);
            }
        }
    };
}


namespace Common::HttpSenderImpl
{
    HttpSender::HttpSender(std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper) : m_curlWrapper(std::move(curlWrapper))
    {
        // Initialising ssl and crypto to create a dependency on their libraries and be able to get the correct rpath
        // TODO: [LINUXEP-6636] Rebuild libcurl with rpath set and remove these lines
        SSL_library_init();
        OPENSSL_init_crypto(OPENSSL_INIT_NO_ADD_ALL_CIPHERS, nullptr);

        CURLcode result = m_curlWrapper->curlGlobalInit(CURL_GLOBAL_DEFAULT); // NOLINT
        if (result != CURLE_OK)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to initialise libcurl with error: " << m_curlWrapper->curlEasyStrError(result);
            throw std::runtime_error(errorMsg.str());
        }
    }

    HttpSender::~HttpSender()
    {
        m_curlWrapper->curlGlobalCleanup();
    }

    CURLcode HttpSender::setCurlOptions(CURL* curl, RequestConfig& requestConfig, curl_slist** headers, std::vector<std::tuple<std::string, CURLoption, std::string>> &curlOptions)
    {


        std::stringstream uriStream;
        uriStream << "https://" << requestConfig.getServer() << ":" << requestConfig.getPort()
                  << requestConfig.getResourceRootAsString();
        std::string uri = uriStream.str();

        LOGINFO("Creating HTTPS " << requestConfig.getRequestTypeAsString() << " Request to " << uri);

        CURLcode result = CURLE_FAILED_INIT;

        curlOptions.emplace_back("Specify network URL", CURLOPT_URL, uri);
        curlOptions.emplace_back(
            "Specify path to Certificate Authority bundle", CURLOPT_CAINFO, requestConfig.getCertPath());

        for (const auto& header : requestConfig.getAdditionalHeaders())
        {
            curl_slist* temp = nullptr;
            temp = m_curlWrapper->curlSlistAppend(*headers, header);
            if (!temp)
            {
                SListScopeGuard sListScopeGuard(*headers, *m_curlWrapper);
                LOGERROR("Failed to append header to request");
                return result;
            }
            *headers = temp;
        }

        if (requestConfig.getRequestType() == RequestType::POST)
        {
            curlOptions.emplace_back(
                "Specify data to POST to server", CURLOPT_COPYPOSTFIELDS, requestConfig.getData());
        }
        else if (requestConfig.getRequestType() == RequestType::PUT)
        {
            curlOptions.emplace_back("Specify a custom PUT request", CURLOPT_CUSTOMREQUEST, "PUT");
            curlOptions.emplace_back(
                "Specify data to PUT to server", CURLOPT_COPYPOSTFIELDS, requestConfig.getData());
        }

        for (const auto& curlOption : curlOptions)
        {
            result = m_curlWrapper->curlEasySetOpt(curl, std::get<1>(curlOption), std::get<2>(curlOption));
            if (result != CURLE_OK)
            {
                LOGERROR("Failed to: " << std::get<0>(curlOption)
                                       << " with error: " << m_curlWrapper->curlEasyStrError(result));
                return result;
            }
        }

        if (*headers)
        {
            result = m_curlWrapper->curlEasySetOptHeaders(curl, *headers);
        }

        return result;
    }

    int HttpSender::doHttpsRequest(RequestConfig& requestConfig)
    {
        CURLcode result;
        curl_slist* headers = nullptr;
        std::vector<std::tuple<std::string, CURLoption, std::string>> curlOptions;
        CURL * curl = m_curlWrapper->curlEasyInit();

        if (!curl)
        {
            LOGERROR("Failed to initialise curl");
            return CURLE_FAILED_INIT;
        }

        CurlScopeGuard curlScopeGuard(curl, *m_curlWrapper);

        result = setCurlOptions(curl, requestConfig, &headers, curlOptions);
        if (result != CURLE_OK)
        {
            LOGERROR("Failed to set curl options with error: " << m_curlWrapper->curlEasyStrError(result));
            return result;
        }

        SListScopeGuard headersScopeGuard(headers, *m_curlWrapper);

        result = m_curlWrapper->curlEasyPerform(curl);
        if (result != CURLE_OK)
        {
            LOGERROR("Failed to perform file transfer with error: " << m_curlWrapper->curlEasyStrError(result));
            return result;
        }
        return static_cast<int>(result);
    }
} // LCOV_EXCL_LINE // namespace Common::HttpSenderImpl
