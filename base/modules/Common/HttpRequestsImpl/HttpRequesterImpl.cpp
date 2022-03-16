
#include "HttpRequesterImpl.h"

#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "CurlFunctionsProvider.h"
#include "CurlWrapper/CurlWrapper.h"

#include <curl/curl.h>

namespace Common::HttpRequestsImpl
{

    HttpRequesterImpl::HttpRequesterImpl(std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper) :
        m_curlWrapper(curlWrapper)
    {
        CURLcode result = m_curlWrapper->curlGlobalInit(CURL_GLOBAL_DEFAULT);
        if (result != CURLE_OK)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to initialise libcurl with error: " << m_curlWrapper->curlEasyStrError(result);
            throw std::runtime_error(errorMsg.str());
        }

        m_curlHandle = m_curlWrapper->curlEasyInit();
        if (!m_curlHandle)
        {
            throw Common::HttpRequests::HttpRequestsException("Failed to initialise curl");
        }

        if (getHttpRequesterImplLogger().getLogLevel() < log4cplus::INFO_LOG_LEVEL)
        {
            m_curlVerboseLogging = true;
        }
    }

    HttpRequesterImpl::~HttpRequesterImpl()
    {
        if (m_curlHandle)
        {
            m_curlWrapper->curlEasyCleanup(m_curlHandle);
        }
        m_curlWrapper->curlGlobalCleanup();
    }

    Common::HttpRequests::Response HttpRequesterImpl::performRequest(Common::HttpRequests::RequestConfig request)
    {
        // Kep the handle but perform a reset. Options are reset to stop any accidental side effects while still
        // persisting: live connections, session ID cache, DNS cache, cookies and the alt-svc cache.
        m_curlWrapper->curlEasyReset(m_curlHandle);

        Common::HttpRequests::Response response;
        if (!m_curlHandle)
        {
            LOGERROR("Failed to initialise curl");
            response.error = "Failed to initialise curl";
            return response;
        }

        std::string bodyBuffer;
        ResponseBuffer responseBuffer;
        responseBuffer.url = request.url;

        curl_easy_setopt(m_curlHandle, CURLOPT_HEADERFUNCTION, CurlFunctionsProvider::curlWriteHeadersFunc);
        curl_easy_setopt(m_curlHandle, CURLOPT_HEADERDATA, &responseBuffer);
        curl_easy_setopt(m_curlHandle, CURLOPT_DEBUGFUNCTION, CurlFunctionsProvider::curlWriteDebugFunc);

        if (request.fileDownloadLocation.has_value())
        {
            if (Common::FileSystem::fileSystem()->isFile(request.fileDownloadLocation.value()))
            {
                response.error = "Download target file name already exists: " + request.fileDownloadLocation.value();
                return response;
            }
            else if (Common::FileSystem::fileSystem()->isDirectory(request.fileDownloadLocation.value()))
            {
                LOGDEBUG("Saving downloaded file to directory: " << request.fileDownloadLocation.value());
                responseBuffer.downloadDirectory = request.fileDownloadLocation.value();
                curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, CurlFunctionsProvider::curlWriteFileFunc);
                curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &responseBuffer);
            }
            else
            {
                LOGDEBUG("Saving downloaded file to file: " << request.fileDownloadLocation.value());
                responseBuffer.downloadFilePath = request.fileDownloadLocation.value();
                curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, CurlFunctionsProvider::curlWriteFileFunc);
                curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &responseBuffer);
            }
        }
        else
        {
            curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, CurlFunctionsProvider::curlWriteFunc);
            curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &bodyBuffer);
        }

        if (request.parameters.has_value())
        {
            std::string urlParamString;
            for (auto const& [param, value] : request.parameters.value())
            {
                LOGDEBUG("Encoding param: " << param << "=" << value);
                if (!urlParamString.empty())
                {
                    urlParamString += "&";
                }
                urlParamString += curlEscape(param);
                urlParamString += "=";
                urlParamString += curlEscape(value);
            }
            if (!urlParamString.empty())
            {
                request.url += "?";
                request.url += urlParamString;
            }
        }

        std::unique_ptr<FILE, int (*)(FILE *)> fileToSend(nullptr, fclose);
        if (request.fileToUpload.has_value())
        {
            if (FileSystem::fileSystem()->isFile(request.fileToUpload.value()))
            {
                curl_easy_setopt(m_curlHandle, CURLOPT_READFUNCTION, CurlFunctionsProvider::curlFileReadFunc);
                fileToSend.reset(fopen(request.fileToUpload.value().c_str(), "rb"));
                if (fileToSend != nullptr)
                {
                    LOGDEBUG("Sending file: "<< request.fileToUpload.value() << ", size: " << FileSystem::fileSystem()->fileSize(request.fileToUpload.value()));
                    curl_easy_setopt(m_curlHandle, CURLOPT_UPLOAD, 1L);
                    curl_easy_setopt(m_curlHandle, CURLOPT_READDATA, fileToSend.get());
                    curl_easy_setopt(m_curlHandle, CURLOPT_INFILESIZE_LARGE, (curl_off_t)FileSystem::fileSystem()->fileSize(request.fileToUpload.value()));
                }
                else
                {
                    response.error = "Failed to open file for reading: " + request.fileToUpload.value();
                    return response;
                }

            }
            else
            {
                response.error ="File to upload does not exist: " + request.fileToUpload.value();
                return response;
            }
        }

        // cURL options are built up in this container and then applied at the end
        std::vector<std::tuple<std::string, CURLoption, std::variant<std::string, long>>> curlOptions;

        curlOptions.emplace_back("URL - CURLOPT_URL", CURLOPT_URL, request.url);
        curlOptions.emplace_back("TLS/SSL version - CURLOPT_SSLVERSION", CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

        if (request.port.has_value())
        {
            curlOptions.emplace_back("Port - CURLOPT_PORT", CURLOPT_PORT, request.port.value());
        }

        switch (request.requestType)
        {
            case Common::HttpRequests::GET:
                curlOptions.emplace_back("GET request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "GET");
                break;
            case Common::HttpRequests::POST:
                curlOptions.emplace_back("POST request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "POST");
                if (request.data.has_value())
                {
                    curlOptions.emplace_back("POST data - CURLOPT_COPYPOSTFIELDS", CURLOPT_COPYPOSTFIELDS, request.data.value());
                }
                break;
            case Common::HttpRequests::PUT:
                curlOptions.emplace_back("PUT request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "PUT");
                if (request.data.has_value())
                {
                    curlOptions.emplace_back("PUT data - CURLOPT_COPYPOSTFIELDS", CURLOPT_COPYPOSTFIELDS, request.data.value());
                }
                break;
            case Common::HttpRequests::DELETE:
                curlOptions.emplace_back("DELETE request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
            case Common::HttpRequests::OPTIONS:
                curlOptions.emplace_back("OPTIONS request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "OPTIONS");
                break;
        }

        if (request.proxy.has_value())
        {
            curlOptions.emplace_back("Specify proxy - CURLOPT_PROXY", CURLOPT_PROXY, request.proxy.value());
            if (request.proxyUsername.has_value() && request.proxyPassword.has_value())
            {
                std::string encodedUsername = curl_easy_escape(
                    m_curlHandle, request.proxyUsername.value().c_str(), request.proxyUsername.value().length());
                std::string encodedPassword = curl_easy_escape(
                    m_curlHandle, request.proxyPassword.value().c_str(), request.proxyPassword.value().length());
                curlOptions.emplace_back(
                    "Set cURL to choose best authentication available - CURLOPT_PROXYAUTH", CURLOPT_PROXYAUTH, CURLAUTH_ANY);
                curlOptions.emplace_back(
                    "Set proxy user and password - CURLOPT_PROXYUSERPWD", CURLOPT_PROXYUSERPWD, encodedUsername + ":" + encodedPassword);
            }
        }

        if (request.allowRedirects)
        {
            long followRedirects = 1;
            curlOptions.emplace_back("Follow redirects - CURLOPT_FOLLOWLOCATION", CURLOPT_FOLLOWLOCATION, followRedirects);
            curlOptions.emplace_back("Set max redirects - CURLOPT_MAXREDIRS", CURLOPT_MAXREDIRS, 50);
        }

        if (request.bandwidthLimit.has_value())
        {
            curlOptions.emplace_back(
                "Set max bandwidth - CURLOPT_MAX_RECV_SPEED_LARGE", CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)request.bandwidthLimit.value());
        }

        if (m_curlVerboseLogging)
        {
            curlOptions.emplace_back("Enable verbose logging - CURLOPT_VERBOSE", CURLOPT_VERBOSE, 1L);
        }

        curlOptions.emplace_back("Set timeout - CURLOPT_TIMEOUT", CURLOPT_TIMEOUT, request.timeout);

        if (request.certPath.has_value())
        {
            LOGINFO("Using client specified CA path: " << request.certPath.value());
            curlOptions.emplace_back("Path for CA bundle - CURLOPT_CAINFO", CURLOPT_CAINFO, request.certPath.value());
        }
        else
        {
            const std::vector<std::string> caPaths = { "/etc/ssl/certs/ca-certificates.crt",
                                                       "/etc/pki/tls/certs/ca-bundle.crt" };
            bool caPathFound = false;
            for (const auto& caPath : caPaths)
            {
                if (Common::FileSystem::fileSystem()->isFile(caPath))
                {
                    caPathFound = true;
                    LOGINFO("Using system CA path: " << caPath);
                    curlOptions.emplace_back("Path for CA bundle - CURLOPT_CAINFO", CURLOPT_CAINFO, caPath);
                    break;
                }
            }

            if (!caPathFound)
            {
                response.error ="No CA paths could be used";
                return response;
            }
        }

        struct curl_slist* curlHeaders = nullptr;

        if (request.headers.has_value())
        {
            for (auto const& [header, value] : request.headers.value())
            {
                LOGDEBUG("Append header: " << header << ":" << value);
                curlHeaders = m_curlWrapper->curlSlistAppend(curlHeaders, header + ":" + value);
                if (!curlHeaders)
                {
                    m_curlWrapper->curlSlistFreeAll(curlHeaders);
                    LOGERROR("Failed to append header to request");
                    response.error = "Failed to append header: " + header;
                    return response;
                }
            }
        }
        Common::CurlWrapper::SListScopeGuard curlListScopeGuard(curlHeaders, *m_curlWrapper);

        // Apply all cURL options
        for (const auto& curlOption : curlOptions)
        {
            LOGDEBUG("Setting cURL option: " << std::get<0>(curlOption));
            CURLcode result =
                m_curlWrapper->curlEasySetOpt(m_curlHandle, std::get<1>(curlOption), std::get<2>(curlOption));

            if (result != CURLE_OK)
            {
                LOGERROR("Failed to set curl option: " << std::get<0>(curlOption));
                response.error = m_curlWrapper->curlEasyStrError(result);
                return response;
            }
        }

        if (curlHeaders)
        {
            CURLcode result = m_curlWrapper->curlEasySetOptHeaders(m_curlHandle, curlHeaders);
            if (result != CURLE_OK)
            {
                LOGERROR("Failed to set headers");
                response.error = m_curlWrapper->curlEasyStrError(result);
                return response;
            }
        }

        try
        {
            LOGDEBUG("Performing request");
            CURLcode result = m_curlWrapper->curlEasyPerform(m_curlHandle);
            LOGDEBUG("Performed request: " << result);
            if (result != CURLE_OK)
            {
                response.error = m_curlWrapper->curlEasyStrError(result);
                return response;
            }
        }
        catch (const std::exception& ex)
        {
            response.error ="Request threw exception: " + std::string(ex.what());
            return response;
        }

        long responseCode = 0;
        response.body = bodyBuffer;
        response.headers = responseBuffer.headers;
        if (m_curlWrapper->curlGetResponseCode(m_curlHandle, &responseCode) == CURLE_OK)
        {
            response.status = responseCode;
        }
        else
        {
            response.error = "Failed to get response code from cURL";
            return response;
        }

        return response;
    }

    Common::HttpRequests::Response HttpRequesterImpl::get(Common::HttpRequests::RequestConfig request)
    {
        request.requestType = Common::HttpRequests::GET;
        return performRequest(request);
    }

    Common::HttpRequests::Response HttpRequesterImpl::post(Common::HttpRequests::RequestConfig request)
    {
        request.requestType = Common::HttpRequests::POST;
        return performRequest(request);
    }

    Common::HttpRequests::Response HttpRequesterImpl::put(Common::HttpRequests::RequestConfig request)
    {
        request.requestType = Common::HttpRequests::PUT;
        return performRequest(request);
    }

    Common::HttpRequests::Response HttpRequesterImpl::del(Common::HttpRequests::RequestConfig request)
    {
        request.requestType = Common::HttpRequests::DELETE;
        return performRequest(request);
    }

    Common::HttpRequests::Response HttpRequesterImpl::options(Common::HttpRequests::RequestConfig request)
    {
        request.requestType = Common::HttpRequests::OPTIONS;
        return performRequest(request);
    }

    std::string HttpRequesterImpl::curlEscape(std::string stringToEscape)
    {
        auto escaped = curl_easy_escape(m_curlHandle, stringToEscape.c_str(), stringToEscape.length());
        if (escaped)
        {
            return escaped;
        }
        throw std::runtime_error("Failed to escape string: " + stringToEscape);
    }

} // namespace Common::HttpRequestsImpl