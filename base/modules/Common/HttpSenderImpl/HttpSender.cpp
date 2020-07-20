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
    struct HoldBody
    {
        std::string body; 
    };

    size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) //NOLINT
    {
        HoldBody * resp = static_cast<HoldBody*>(userp); 
        char* data = static_cast<char*>(buffer); 
        resp->body += std::string{data, data + (size*nmemb)};
        return nmemb; 
    }

}

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
    using namespace Common::HttpSender;
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

    int HttpSender::doHttpsRequest(const RequestConfig& requestConfig)
    {
        long curlCode; 
        fetchHttpRequest(requestConfig, false, &curlCode); 
        return static_cast<int>(curlCode);   
    }

    Common::HttpSenderImpl::HttpResponse HttpSender::fetchHttpRequest(const RequestConfig& requestConfig, bool captureBody, long* curlCode)
    {
        using HttResponse = Common::HttpSender::HttpResponse; 
        auto onError=[this, curlCode](int errorCode) -> HttResponse{ *curlCode = errorCode; return HttpResponse{errorCode,  m_curlWrapper->curlEasyStrError(static_cast<CURLcode>(errorCode))}; }; 
        curl_slist* headers = nullptr;
        LOGINFO("Running easyPerform early code ");
        std::vector<std::tuple<std::string, CURLoption, std::variant<std::string, long>> > curlOptions;

        CURL* curl = m_curlWrapper->curlEasyInit();

        if (!curl)
        {
            LOGERROR("Failed to initialise curl");
            *curlCode=CURLE_FAILED_INIT;             
            return HttResponse{CURLE_FAILED_INIT};
        }
        HoldBody holdBody; 
        if (captureBody)
        {
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &holdBody);
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
                *curlCode = CURLE_FAILED_INIT;
                return HttResponse{CURLE_FAILED_INIT};
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
                HttpResponse resp = onError(result); 
                LOGERROR(
                    "Failed to: " << std::get<0>(curlOption)
                                  << " with error: " << resp.description);
                return resp;
            }
        }

        if (headers)
        {
            CURLcode result = m_curlWrapper->curlEasySetOptHeaders(curl, headers);

            if (result != CURLE_OK)
            {
                HttpResponse resp = onError(result); 
                LOGERROR("Failed to set headers with error: " << resp.description);
                return resp;
            }
        }

        LOGDEBUG("Running easyPerform. ");
        CURLcode result = m_curlWrapper->curlEasyPerform(curl);
        LOGDEBUG("Performed easyPerform: " << result);
        if (result != CURLE_OK)
        {
            HttpResponse resp = onError(result); 
            LOGERROR("Failed to perform file transfer with error: " << resp.description);
            return resp;
        }
        else{
            *curlCode = CURLE_OK;
            long response_code;
            if (m_curlWrapper->curlGetResponseCode(curl, &response_code) == CURLE_OK)
            {
                HttpResponse httpResponse{static_cast<int>(response_code)};
                httpResponse.bodyContent = std::move(holdBody.body); 
                return httpResponse; 
            }             
        }
        return HttpResponse{result};        

    }
} // namespace Common::HttpSenderImpl
