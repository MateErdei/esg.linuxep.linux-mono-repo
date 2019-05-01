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

namespace Common
{
    namespace HttpSender
    {
        HttpSender::HttpSender(std::shared_ptr<ICurlWrapper> curlWrapper) : m_curlWrapper(std::move(curlWrapper))
        {
            // Initialising ssl and crypto to create a dependency on their libraries and be able to get the correct rpath
            // TODO: [LINUXEP-6636] Rebuild libcurl with rpath set and remove these lines
            SSL_library_init();
            OPENSSL_init_crypto(OPENSSL_INIT_NO_ADD_ALL_CIPHERS, nullptr);
        }

        curl_slist* HttpSender::setCurlOptions(CURL* curl, const std::shared_ptr<RequestConfig>& requestConfig)
        {
            curl_slist* headers = nullptr;

            std::stringstream uriStream;
            uriStream << "https://" << requestConfig->getServer() << ":" << requestConfig->getPort()
                      << requestConfig->getResourceRootAsString();
            std::string uri = uriStream.str();

            LOGINFO("Creating HTTPS " << requestConfig->getRequestTypeAsString() << " Request to " << uri);

            CURLcode result;
            std::vector<std::tuple<const char*, CURLoption, const char*>> curlOptions;

            curlOptions.emplace_back("Specify network URL", CURLOPT_URL, uri.c_str());
            curlOptions.emplace_back(
                "Specify path to Certificate Authority bundle", CURLOPT_CAINFO, requestConfig->getCertPath().c_str());

            for (const auto& header : requestConfig->getAdditionalHeaders())
            {
                headers = m_curlWrapper->curlSlistAppend(headers, header.c_str());
                if (!headers)
                {
                    throw std::runtime_error("Failed to append header to request");
                }
            }

            if (headers)
            {
                curlOptions.emplace_back(
                    "Specify custom HTTP header(s)", CURLOPT_HTTPHEADER, reinterpret_cast<const char*>(headers));
            }

            if (requestConfig->getRequestType() == RequestType::POST)
            {
                curlOptions.emplace_back(
                    "Specify data to POST to server", CURLOPT_POSTFIELDS, requestConfig->getData().c_str());
            }
            else if (requestConfig->getRequestType() == RequestType::PUT)
            {
                curlOptions.emplace_back("Specify a custom PUT request", CURLOPT_CUSTOMREQUEST, "PUT");
                curlOptions.emplace_back(
                    "Specify data to POST to server", CURLOPT_POSTFIELDS, requestConfig->getData().c_str());
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

            return headers;
        }

        int HttpSender::doHttpsRequest(std::shared_ptr<RequestConfig> requestConfig)
        {
            CURL* curl = nullptr;
            CURLcode result;
            curl_slist* headers = nullptr;

            try
            {
                result = m_curlWrapper->curlGlobalInit(CURL_GLOBAL_DEFAULT); // NOLINT

                if (result != CURLE_OK)
                {
                    std::stringstream errorMsg;
                    errorMsg << "Failed to initialise libcurl with error: " << m_curlWrapper->curlEasyStrError(result);
                    throw std::runtime_error(errorMsg.str());
                }

                curl = m_curlWrapper->curlEasyInit();
                if (curl)
                {
                    headers = setCurlOptions(curl, requestConfig);
                    result = m_curlWrapper->curlEasyPerform(curl);

                    if (result != CURLE_OK)
                    {
                        std::stringstream errorMsg;
                        errorMsg << "Failed to perform file transfer with error: "
                                 << m_curlWrapper->curlEasyStrError(result);
                        throw std::runtime_error(errorMsg.str());
                    }

                    if (headers)
                    {
                        m_curlWrapper->curlSlistFreeAll(headers);
                    }
                    m_curlWrapper->curlEasyCleanup(curl);
                }
                else
                {
                    throw std::runtime_error("Failed to initialise curl");
                }

                m_curlWrapper->curlGlobalCleanup();
            }

            catch (const std::runtime_error& e)
            {
                result = CURLE_FAILED_INIT;
                LOGERROR(
                    "Exception while making HTTPS " << requestConfig->getRequestTypeAsString()
                                                    << " request: " << e.what());
                if (headers)
                {
                    m_curlWrapper->curlSlistFreeAll(headers);
                }
                m_curlWrapper->curlEasyCleanup(curl);
                m_curlWrapper->curlGlobalCleanup();
            }

            return static_cast<int>(result);
        }
    } // namespace HttpSender
} // namespace Common
