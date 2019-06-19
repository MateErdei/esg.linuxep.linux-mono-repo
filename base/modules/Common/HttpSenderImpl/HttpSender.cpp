/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "HttpSender.h"

#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
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

    int HttpSender::doHttpsRequest(RequestConfig& requestConfig)
    {
        curl_slist* headers = nullptr;

        std::vector<std::tuple<std::string, CURLoption, std::string>> curlOptions;

        CURL * curl = m_curlWrapper->curlEasyInit();

        if (!curl)
        {
            LOGERROR("Failed to initialise curl");
            return CURLE_FAILED_INIT;
        }

        CurlScopeGuard curlScopeGuard(curl, *m_curlWrapper);

        std::stringstream uriStream;
        uriStream << "https://" << requestConfig.getServer() << ":" << requestConfig.getPort() << "/"
                  << requestConfig.getResourcePath();
        std::string uri = uriStream.str();

        LOGINFO("Creating HTTPS " << requestConfig.getRequestTypeAsString() << " Request to " << uri);

        curlOptions.emplace_back("Specify network URL", CURLOPT_URL, uri);
        std::string certPath = requestConfig.getCertPath();

        if (certPath.empty())
        {
            const std::vector<Path> caDirs = { "/etc/ssl/certs", "/etc/pki/tls/certs" };

            for (const auto& caDir : caDirs)
            {
                if (FileSystem::fileSystem()->isDirectory(caDir))
                {
                    std::vector<Path> files = FileSystem::fileSystem()->listFiles(caDir);

                    if (!files.empty())
                    {
                        LOGINFO("Using Certificate Authority path: " << caDir);
                        curlOptions.emplace_back(
                            "Library specified directory for Certificate Authority bundle", CURLOPT_CAPATH, caDir);
                        break;
                    }
                }
            }
        }
        else
        {
            LOGINFO("Using Certificate Authority path: " << certPath);
            curlOptions.emplace_back(
                "Client specified path for Certificate Authority bundle", CURLOPT_CAINFO, certPath);
        }

        for (const auto& header : requestConfig.getAdditionalHeaders())
        {
            curl_slist* temp = nullptr;
            temp = m_curlWrapper->curlSlistAppend(headers, header);
            if (!temp)
            {
                m_curlWrapper->curlSlistFreeAll(headers);
                LOGERROR("Failed to append header to request");
                return CURLE_FAILED_INIT;
            }
            headers = temp;
        }

        SListScopeGuard sListScopeGuard(headers, *m_curlWrapper);

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
            CURLcode result = m_curlWrapper->curlEasySetOpt(curl, std::get<1>(curlOption), std::get<2>(curlOption));

            if (result != CURLE_OK)
            {
                LOGERROR("Failed to: " << std::get<0>(curlOption)
                                       << " with error: " << m_curlWrapper->curlEasyStrError(result));
                return result;
            }
        }

        if (headers)
        {
            CURLcode result = m_curlWrapper->curlEasySetOptHeaders(curl, headers);

            if (result != CURLE_OK)
            {
                LOGERROR("Failed to set headers with error: " << m_curlWrapper->curlEasyStrError(result));
                return result;
            }
        }

        CURLcode result = m_curlWrapper->curlEasyPerform(curl);

        if (result != CURLE_OK)
        {
            LOGERROR("Failed to perform file transfer with error: " << m_curlWrapper->curlEasyStrError(result));
            return result;
        }

        return static_cast<int>(result);
    }
} // LCOV_EXCL_LINE // namespace Common::HttpSenderImpl
