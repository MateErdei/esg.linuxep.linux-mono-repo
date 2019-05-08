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
        Common::HttpSender::ICurlWrapper & m_iCurlWrapper;
        CURL * m_curl;
    public:

        CurlScopeGuard( CURL* curl, Common::HttpSender::ICurlWrapper& iCurlWrapper ):
            m_iCurlWrapper(iCurlWrapper), m_curl(curl)
        {

        }
        ~CurlScopeGuard()
        {
            m_iCurlWrapper.curlEasyCleanup(m_curl);
        }

    };

    class SListScopeGuard
    {
        Common::HttpSender::ICurlWrapper & m_iCurlWrapper;
        curl_slist  * m_curl_slist;
    public:

        SListScopeGuard( curl_slist * curl_slist, Common::HttpSender::ICurlWrapper& iCurlWrapper ):
        m_iCurlWrapper(iCurlWrapper), m_curl_slist (curl_slist)
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

    void HttpSender::setCurlOptions(CURL* curl, std::shared_ptr<RequestConfig> requestConfig, std::vector<std::tuple<std::string, CURLoption, std::string>>& curlOptions)
    {
        curl_slist* headers = nullptr;

        std::stringstream uriStream;
        uriStream << "https://" << requestConfig->getServer() << ":" << requestConfig->getPort()
                  << requestConfig->getResourceRootAsString();
        std::string uri = uriStream.str();

        LOGINFO("Creating HTTPS " << requestConfig->getRequestTypeAsString() << " Request to " << uri);

        CURLcode result;

        curlOptions.emplace_back("Specify network URL", CURLOPT_URL, uri);
        curlOptions.emplace_back(
            "Specify path to Certificate Authority bundle", CURLOPT_CAINFO, requestConfig->getCertPath());

        for (const auto& header : requestConfig->getAdditionalHeaders())
        {
            curl_slist* temp = nullptr;
            temp = m_curlWrapper->curlSlistAppend(headers, header);
            if (!temp)
            {
                SListScopeGuard sListScopeGuard( headers, *m_curlWrapper);
                throw std::runtime_error("Failed to append header to request");
            }
            headers = temp;
        }
        SListScopeGuard headersScopeGuard(headers, *m_curlWrapper);
        if (headers)
        {
            curlOptions.emplace_back(
                "Specify custom HTTP header(s)", CURLOPT_HTTPHEADER, reinterpret_cast<const char*>(headers));
        }

        if (requestConfig->getRequestType() == RequestType::POST)
        {
            curlOptions.emplace_back(
                "Specify data to POST to server", CURLOPT_POSTFIELDS, requestConfig->getData());
        }
        else if (requestConfig->getRequestType() == RequestType::PUT)
        {
            curlOptions.emplace_back("Specify a custom PUT request", CURLOPT_CUSTOMREQUEST, "PUT");
            curlOptions.emplace_back(
                "Specify data to POST to server", CURLOPT_POSTFIELDS, requestConfig->getData());
        }

        for (const auto& curlOption : curlOptions)
        {
            result = m_curlWrapper->curlEasySetOpt(curl, std::get<1>(curlOption), std::get<2>(curlOption));
            if (result != CURLE_OK)
            {
                std::stringstream errorMsg;
                errorMsg << "Failed to: " << std::get<0>(curlOption)
                         << " with error: " << m_curlWrapper->curlEasyStrError(result);
                throw std::runtime_error(errorMsg.str());
            }
        }

    }

    int HttpSender::doHttpsRequest(std::shared_ptr<RequestConfig> requestConfig)
    {
        CURLcode result;
        std::vector<std::tuple<std::string, CURLoption, std::string>> curlOptions;

        try
        {
            CURL * curl = m_curlWrapper->curlEasyInit();
            if (curl)
            {
                CurlScopeGuard curlScopeGuard(curl, *m_curlWrapper);
                setCurlOptions(curl, requestConfig, curlOptions);
                result = m_curlWrapper->curlEasyPerform(curl);

                if (result != CURLE_OK)
                {

                    std::stringstream errorMsg;
                    errorMsg << "Failed to perform file transfer with error: "
                             << m_curlWrapper->curlEasyStrError(result);
                    throw std::runtime_error(errorMsg.str());
                }
            }
            else
            {
                throw std::runtime_error("Failed to initialise curl");
            }
        }

        catch (const std::runtime_error& e)
        {
            result = CURLE_FAILED_INIT;
            LOGERROR(
                "Exception while making HTTPS " << requestConfig->getRequestTypeAsString()
                                                << " request: " << e.what());
        }

        return static_cast<int>(result);
    }
} // namespace Common::HttpSenderImpl
