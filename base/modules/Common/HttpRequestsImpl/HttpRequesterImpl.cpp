
#include "HttpRequesterImpl.h"

#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <curl/curl.h>

namespace
{

    // TODO m ove curl functions to another provider class.

    size_t curlWriteFunc(void* ptr, size_t size, size_t nmemb, std::string* buffer)
    {
        size_t totalSizeBytes = size * nmemb;
        buffer->append(static_cast<char*>(ptr), totalSizeBytes);
        return totalSizeBytes;
    }

    size_t curlWriteFileFunc(void* ptr, size_t size, size_t nmemb, Common::HttpRequestsImpl::ResponseBuffer* responseBuffer)
    {
        size_t totalSizeBytes = size * nmemb;
        std::string content;
        content.append(static_cast<char*>(ptr), totalSizeBytes);
        auto fileSystem = Common::FileSystem::fileSystem();
        if (responseBuffer->downloadFilePath.empty())
        {
            if (responseBuffer->downloadDirectory.empty())
            {
                throw std::runtime_error("Neither download target filename or directory specified");
            }

            std::string contentDispositionHeaderTag = "Content-Disposition";

            // If no download filename specified then we need to work it out from headers or URL
            if (responseBuffer->headers.count(contentDispositionHeaderTag))
            {
                // Work out filename from Content-disposition header
                std::string filenameTag = "filename";
                /* Example Content-Disposition: filename=name1367; charset=funny; option=strange */
                if (Common::UtilityImpl::StringUtils::isSubstring(responseBuffer->headers[contentDispositionHeaderTag], "filename"))
                {
                    std::vector<std::string> contentOptiopns = Common::UtilityImpl::StringUtils::splitString(responseBuffer->headers[contentDispositionHeaderTag], ";");
                    for (const auto& option: contentOptiopns)
                    {
                        if (Common::UtilityImpl::StringUtils::isSubstring(option, "="))
                        {
                            std::vector<std::string> optionAndValue = Common::UtilityImpl::StringUtils::splitString(option, "=");
                            if (optionAndValue.size() == 2)
                            {
                                std::function isWhitespaceOrQuote = [](char c) { return std::isspace(c) || c == '"'; };
                                std::string cleanOption = Common::UtilityImpl::StringUtils::trim(optionAndValue[0], isWhitespaceOrQuote);
                                std::string cleanValue = Common::UtilityImpl::StringUtils::trim(optionAndValue[1], isWhitespaceOrQuote);
                                if (cleanOption == filenameTag && !cleanValue.empty())
                                {
                                    responseBuffer->downloadFilePath = Common::FileSystem::join(responseBuffer->downloadDirectory, cleanValue);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                // Work out filename from URL
                responseBuffer->downloadFilePath = Common::FileSystem::join(responseBuffer->downloadDirectory, Common::FileSystem::basename(responseBuffer->url));
            }
        }

        if (responseBuffer->downloadFilePath.empty())
        {
            throw std::runtime_error("Could not work out download file name to use");
        }

        LOGDEBUG("Writing download to: " << responseBuffer->downloadFilePath);
        try
        {
            fileSystem->appendFile(responseBuffer->downloadFilePath, content);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            // TODO throw exception from Interface
            LOGERROR("Failed to append data to file: " << responseBuffer->downloadFilePath << ", error: " << ex.what());
        }
        return totalSizeBytes;
    }

    size_t curlWriteHeadersFunc(void* ptr, size_t size, size_t nmemb, Common::HttpRequestsImpl::ResponseBuffer* responseBuffer)
    {
        size_t totalSizeBytes = size * nmemb;
        std::string fullHeaderString;
        fullHeaderString.append(static_cast<char*>(ptr), totalSizeBytes);
        size_t colonDeliminatorPos = fullHeaderString.find(':');
        if (colonDeliminatorPos != std::string::npos && fullHeaderString.length() > colonDeliminatorPos)
        {
            responseBuffer->headers.emplace(
                Common::UtilityImpl::StringUtils::trim(fullHeaderString.substr(0, colonDeliminatorPos)),
                Common::UtilityImpl::StringUtils::trim(
                    fullHeaderString.substr(colonDeliminatorPos + 1, fullHeaderString.length())));
        }
        return totalSizeBytes;
    }

    int curlWriteDebugFunc(
        [[maybe_unused]] CURL* handle,
        curl_infotype type,
        char* data,
        size_t size,
        [[maybe_unused]] void* userp)
    {
        std::string debugLine = "cURL ";
        switch (type)
        {
            case CURLINFO_TEXT:
                debugLine += "Info: ";
                break;
            case CURLINFO_HEADER_OUT:
                debugLine += "=> Send header: ";
                break;
            case CURLINFO_DATA_OUT:
                debugLine += "=> Send data: ";
                break;
            case CURLINFO_SSL_DATA_OUT:
                debugLine += "=> Send SSL data: ";
                break;
            case CURLINFO_HEADER_IN:
                debugLine += "<= Recv header: ";
                break;
            case CURLINFO_DATA_IN:
                debugLine += "<= Recv data: ";
                break;
            case CURLINFO_SSL_DATA_IN:
                debugLine += "<= Recv SSL data: ";
                break;
            default:
                LOGDEBUG("Unsupported cURL debug info type");
                return 0;
        }
        debugLine.append(data, size);
        LOGDEBUG(debugLine);
        return 0;
    }

    class SListScopeGuard
    {
        Common::CurlWrapper::ICurlWrapper& m_iCurlWrapper;
        curl_slist* m_curl_slist;

    public:
        SListScopeGuard(curl_slist* curl_slist, Common::CurlWrapper::ICurlWrapper& iCurlWrapper) :
            m_iCurlWrapper(iCurlWrapper), m_curl_slist(curl_slist)
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

namespace Common::HttpRequestsImpl
{

    HttpRequesterImpl::HttpRequesterImpl(std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper) :
        m_curlWrapper(curlWrapper)
    {
        // TODO need to make global init only be able to be called once if multi threaded.
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
            LOGERROR("Failed to initialise curl");
            //        *curlCode = CURLE_FAILED_INIT;
            //        return HttResponse{ CURLE_FAILED_INIT };
            throw std::runtime_error("Failed to initialise curl");
        }

        if (getHttpRequesterImplLogger().getLogLevel() < log4cplus::INFO_LOG_LEVEL)
        {
            m_curlVerboseLogging = true;
        }
    }

    Common::HttpRequests::Response HttpRequesterImpl::performRequest(Common::HttpRequests::RequestConfig request)
    {

        // TODO handle file upload.


        curl_easy_reset(m_curlHandle);

        Common::HttpRequests::Response response;
        if (!m_curlHandle)
        {
            LOGERROR("Failed to initialise curl");
            throw std::runtime_error("Failed to initialise curl");
        }
        std::string bodyBuffer;
        ResponseBuffer responseBuffer;
        responseBuffer.url = request.url;

        curl_easy_setopt(m_curlHandle, CURLOPT_HEADERFUNCTION, curlWriteHeadersFunc);
        curl_easy_setopt(m_curlHandle, CURLOPT_HEADERDATA, &responseBuffer);
        curl_easy_setopt(m_curlHandle, CURLOPT_DEBUGFUNCTION, curlWriteDebugFunc);

        if (request.fileDownloadLocation.has_value())
        {
            if (Common::FileSystem::fileSystem()->isFile(request.fileDownloadLocation.value()))
            {
                throw std::runtime_error("Download target file name already exists: " + request.fileDownloadLocation.value());
            }
            else if (Common::FileSystem::fileSystem()->isDirectory(request.fileDownloadLocation.value()))
            {
                LOGDEBUG("Saving downloaded file to directory: " << request.fileDownloadLocation.value());
                responseBuffer.downloadDirectory = request.fileDownloadLocation.value();
                curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, curlWriteFileFunc);
                curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &responseBuffer);
            }
            else
            {
                LOGDEBUG("Saving downloaded file to file: " << request.fileDownloadLocation.value());
                responseBuffer.downloadFilePath = request.fileDownloadLocation.value();
                curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, curlWriteFileFunc);
                curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &responseBuffer);
            }
        }
        else
        {
            curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, curlWriteFunc);
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

        std::vector<std::tuple<std::string, CURLoption, std::variant<std::string, long>>> curlOptions;

        curlOptions.emplace_back("Specify URL", CURLOPT_URL, request.url);

        //    curlOptions.emplace_back("Specify preferred HTTP version", CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        curlOptions.emplace_back("Specify TLS/SSL version", CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

        if (request.port.has_value())
        {
            curlOptions.emplace_back("Specify port", CURLOPT_PORT, request.port.value());
        }

        switch (request.requestType)
        {
            case Common::HttpRequests::GET:
                curlOptions.emplace_back("Specify GET request", CURLOPT_CUSTOMREQUEST, "GET");
                break;
            case Common::HttpRequests::POST:
                curlOptions.emplace_back("Specify POST request", CURLOPT_CUSTOMREQUEST, "POST");
                if (request.data.has_value())
                {
                    curlOptions.emplace_back("Specify POST data", CURLOPT_COPYPOSTFIELDS, request.data.value());
                }
                break;
            case Common::HttpRequests::PUT:
                curlOptions.emplace_back("Specify a custom PUT request", CURLOPT_CUSTOMREQUEST, "PUT");
                if (request.data.has_value())
                {
                    curlOptions.emplace_back("Specify PUT data", CURLOPT_COPYPOSTFIELDS, request.data.value());
                }
                break;
            case Common::HttpRequests::DELETE:
                throw std::runtime_error("not implemented");
                break;
            case Common::HttpRequests::OPTIONS:
                throw std::runtime_error("not implemented");
                break;
        }

        if (request.proxy.has_value())
        {
            curlOptions.emplace_back("Specify proxy to use", CURLOPT_PROXY, request.proxy.value());
            if (request.proxyUsername.has_value() && request.proxyPassword.has_value())
            {
                std::string encodedUsername = curl_easy_escape(
                    m_curlHandle, request.proxyUsername.value().c_str(), request.proxyUsername.value().length());
                std::string encodedPassword = curl_easy_escape(
                    m_curlHandle, request.proxyPassword.value().c_str(), request.proxyPassword.value().length());
                curlOptions.emplace_back(
                    "Set cURL to choose best authentication available", CURLOPT_PROXYAUTH, CURLAUTH_ANY);
                curlOptions.emplace_back(
                    "Set proxy user and password", CURLOPT_PROXYUSERPWD, encodedUsername + ":" + encodedPassword);
            }
        }

        if (request.allowRedirects)
        {
            long followRedirects = 1;
            curlOptions.emplace_back("Follow redirects", CURLOPT_FOLLOWLOCATION, followRedirects);
            curlOptions.emplace_back("Set max redirects", CURLOPT_MAXREDIRS, 50);
        }

        if (request.bandwidthLimit.has_value())
        {
            curlOptions.emplace_back(
                "Set max bandwidth", CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)request.bandwidthLimit.value());
        }

        if (m_curlVerboseLogging)
        {
            curlOptions.emplace_back("Enable verbose logging", CURLOPT_VERBOSE, 1L);
        }

        curlOptions.emplace_back("Set timeout", CURLOPT_TIMEOUT, request.timeout);

        if (request.certPath.has_value())
        {
            LOGINFO("Using client specified CA path: " << request.certPath.value());
            curlOptions.emplace_back("Path for CA bundle", CURLOPT_CAINFO, request.certPath.value());
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
        SListScopeGuard curlListScopeGuard(curlHeaders, *m_curlWrapper);

        // Apply all cURL options
        for (const auto& curlOption : curlOptions)
        {
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

        LOGDEBUG("Running easyPerform");
        CURLcode result = m_curlWrapper->curlEasyPerform(m_curlHandle);
        LOGDEBUG("Performed easyPerform: " << result);
        if (result != CURLE_OK)
        {
            LOGERROR("Request failed");
            response.error = m_curlWrapper->curlEasyStrError(result);
            //        HttpResponse resp = onError(result);
            //        LOGERROR("Failed to perform file transfer with error: " << resp.description);
            //        return resp;
            return response;
        }

        //    long response_code;
        //    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        long responseCode;
        //    LOGINFO(m_curlWrapper->curlGetResponseCode(curl, &responseCode));
        //    response.status = response_code;
        response.body = bodyBuffer;
        response.headers = responseBuffer.headers;
        //        *curlCode = CURLE_OK;
        //        long response_code;
        if (m_curlWrapper->curlGetResponseCode(m_curlHandle, &responseCode) == CURLE_OK)
        {
            response.status = responseCode;
            //            HttpResponse httpResponse{ static_cast<int>(response_code) };
            //            std::vector<std::string> debugLines = Common::UtilityImpl::StringUtils::splitString(debugLog,
            //            "\n"); for (auto line : debugLines)
            //            {
            //                LOGDEBUG(line);
            //            }
            //
            //            httpResponse.bodyContent = holdBody;
            //            return httpResponse;
        }

        //    return HttpResponse{ result };

        //    CURL *curl;
        //    CURLcode res;
        //
        //    curl = curl_easy_init();
        //    if(curl) {
        //        curl_easy_setopt(curl, CURLOPT_URL, "https://sophos.com");
        //        /* example.com is redirected, so we tell libcurl to follow redirection */
        //        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        //
        //        /* Perform the request, res will get the return code */
        //        res = curl_easy_perform(curl);
        //        /* Check for errors */
        //        if(res != CURLE_OK)
        //            fprintf(stderr, "curl_easy_perform() failed: %s\n",
        //                    curl_easy_strerror(res));
        //
        //
        //        /* always cleanup */
        //        curl_easy_cleanup(curl);
        //    }
        return response;
    }

    // Common::HttpRequests::Response HttpRequesterImpl::get(std::string url)
    //{
    //     return performRequest({ .url = url, .requestType = RequestType::GET });
    // }

    // Common::HttpRequests::Response HttpRequesterImpl::post(std::string url, std::string data)
    //{
    //     RequestConfig request;
    //     request.url = url;
    //     request.data = data;
    //     request.requestType = POST;
    //     return performRequest(request);
    // }

    HttpRequesterImpl::~HttpRequesterImpl()
    {
        if (m_curlHandle)
        {
            m_curlWrapper->curlEasyCleanup(m_curlHandle);
        }
        m_curlWrapper->curlGlobalCleanup();
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
        (void)request;
        return Common::HttpRequests::Response();
    }
    Common::HttpRequests::Response HttpRequesterImpl::options(Common::HttpRequests::RequestConfig request)
    {
        (void)request;
        return Common::HttpRequests::Response();
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