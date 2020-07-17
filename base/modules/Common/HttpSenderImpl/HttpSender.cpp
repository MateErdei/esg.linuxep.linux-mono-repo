/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "HttpSender.h"

#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/LoggerConfig.h>

#include <curl.h>
#include <map>
#include <sstream>
#include <variant>

namespace
{
    class CurlScopeGuard
    {
        Common::HttpSender::ICurlWrapper& m_iCurlWrapper;
        CURL* m_curl;

    public:
        CurlScopeGuard(CURL* curl, Common::HttpSender::ICurlWrapper& iCurlWrapper) :
            m_iCurlWrapper(iCurlWrapper),
            m_curl(curl)
        {
        }

        ~CurlScopeGuard() { m_iCurlWrapper.curlEasyCleanup(m_curl); }
    };

    class SListScopeGuard
    {
        Common::HttpSender::ICurlWrapper& m_iCurlWrapper;
        curl_slist* m_curl_slist;

    public:
        SListScopeGuard(curl_slist* curl_slist, Common::HttpSender::ICurlWrapper& iCurlWrapper) :
            m_iCurlWrapper(iCurlWrapper),
            m_curl_slist(curl_slist)
        {
        }

        ~SListScopeGuard()
        {
            if (m_curl_slist != nullptr)
            {
                m_iCurlWrapper.curlSlistFreeAll(m_curl_slist);
            }
        }
    };
} // namespace

namespace Common::HttpSenderImpl
{
    HttpSender::HttpSender(std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper) :
        m_curlWrapper(std::move(curlWrapper))
    {
        CURLcode result = m_curlWrapper->curlGlobalInit(CURL_GLOBAL_DEFAULT); // NOLINT
        if (result != CURLE_OK)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to initialise libcurl with error: " << m_curlWrapper->curlEasyStrError(result);
            throw std::runtime_error(errorMsg.str());
        }
    }

    HttpSender::~HttpSender() { m_curlWrapper->curlGlobalCleanup(); }

    int HttpSender::doHttpsRequest(Common::HttpSender::RequestConfig& requestConfig)
    {
        using namespace Common::HttpSender;
        curl_slist* headers = nullptr;

        std::vector<std::tuple<std::string, CURLoption, std::variant<std::string, long>> > curlOptions;

        CURL* curl = m_curlWrapper->curlEasyInit();

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
        curlOptions.emplace_back("Specify preferred TLS/SSL version", CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        long verbose = 1L;
        int logLevel = getHttpSenderImplLogger().getLogLevel();

        if (logLevel > 10000)
        {
            verbose = 0L;
        }

        curlOptions.emplace_back("Set logging options", CURLOPT_VERBOSE, verbose);
        std::string certPath = requestConfig.getCertPath();

        if (certPath.empty())
        {
            const std::vector<Path> caPaths = { "/etc/ssl/certs/ca-certificates.crt",
                                                "/etc/pki/tls/certs/ca-bundle.crt" };

            bool caPathFound = false;

            for (const auto& caPath : caPaths)
            {
                if (FileSystem::fileSystem()->isFile(caPath))
                {
                    caPathFound = true;
                    LOGINFO("Using library specified CA path: " << caPath);
                    curlOptions.emplace_back("Path for CA bundle", CURLOPT_CAINFO, caPath);
                    break;
                }
            }

            if (!caPathFound)
            {
                LOGWARN("CA path not found");
            }
        }
        else
        {
            LOGINFO("Using client specified CA path: " << certPath);
            curlOptions.emplace_back("Path for CA bundle", CURLOPT_CAINFO, certPath);
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
            curlOptions.emplace_back("Specify data to POST to server", CURLOPT_COPYPOSTFIELDS, requestConfig.getData());
        }
        else if (requestConfig.getRequestType() == RequestType::PUT)
        {
            curlOptions.emplace_back("Specify a custom PUT request", CURLOPT_CUSTOMREQUEST, "PUT");
            curlOptions.emplace_back("Specify data to PUT to server", CURLOPT_COPYPOSTFIELDS, requestConfig.getData());
        }

        for (const auto& curlOption : curlOptions)
        {
            CURLcode result = m_curlWrapper->curlEasySetOpt(curl, std::get<1>(curlOption), std::get<2>(curlOption));

            if (result != CURLE_OK)
            {
                LOGERROR(
                    "Failed to: " << std::get<0>(curlOption)
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
} // namespace Common::HttpSenderImpl
