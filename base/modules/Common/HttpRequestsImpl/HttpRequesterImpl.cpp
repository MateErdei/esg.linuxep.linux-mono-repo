// Copyright 2022-2024 Sophos Limited. All rights reserved.

#include "HttpRequesterImpl.h"

#include "CurlFunctionsProvider.h"
#include "Logger.h"

#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <curl/curl.h>

#include <utility>

namespace Common::HttpRequestsImpl
{

    HttpRequesterImpl::HttpRequesterImpl(std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper) :
        m_curlWrapper(std::move(curlWrapper))
    {
        CURLcode result = m_curlWrapper->curlGlobalInit(CURL_GLOBAL_DEFAULT);
        if (result != CURLE_OK)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to initialise libcurl with error: " << m_curlWrapper->curlEasyStrError(result);
            throw Common::HttpRequests::HttpRequestsException(errorMsg.str());
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
        // Keep the handle but perform a reset. Options are reset to stop any accidental side effects while still
        // persisting: live connections, session ID cache, DNS cache, cookies and the alt-svc cache.
        m_curlWrapper->curlEasyReset(m_curlHandle);

        Common::HttpRequests::Response response;
        if (!m_curlHandle)
        {
            LOGERROR("Failed to initialise curl");
            response.error = "Failed to initialise curl";
            response.errorCode = HttpRequests::ResponseErrorCode::FAILED;
            return response;
        }

        CurlFunctionsProvider::WriteBackBuffer bodyBuffer;
        if (request.maxSize.has_value())
        {
            bodyBuffer.maxSize_ = request.maxSize.value();
        }

        ResponseBuffer responseBuffer;
        responseBuffer.url = request.url;

        // cURL options are built up in this container and then applied at the end
        std::vector<std::tuple<std::string, CURLoption, std::variant<std::string, long>>> curlOptions;

        m_curlWrapper->curlEasySetFuncOpt(
            m_curlHandle, CURLOPT_HEADERFUNCTION, (void*)CurlFunctionsProvider::curlWriteHeadersFunc);
        m_curlWrapper->curlEasySetDataOpt(m_curlHandle, CURLOPT_HEADERDATA, &responseBuffer);
        m_curlWrapper->curlEasySetFuncOpt(
            m_curlHandle, CURLOPT_DEBUGFUNCTION, (void*)CurlFunctionsProvider::curlWriteDebugFunc);

        // Handle data being downloaded, directly to buffer or to a file.
        if (request.fileDownloadLocation.has_value())
        {
            // If file already exists, then return error
            if (Common::FileSystem::fileSystem()->isFile(request.fileDownloadLocation.value()))
            {
                response.error = "Download target file name already exists: " + request.fileDownloadLocation.value();
                response.errorCode = HttpRequests::ResponseErrorCode::DOWNLOAD_TARGET_ALREADY_EXISTS;
                return response;
            } // If the path is to a directory then we set downloadDirectory (and not downloadFilePath)
            else if (Common::FileSystem::fileSystem()->isDirectory(request.fileDownloadLocation.value()))
            {
                LOGDEBUG("Saving downloaded file to directory: " << request.fileDownloadLocation.value());
                responseBuffer.downloadDirectory = request.fileDownloadLocation.value();
            }
            else // Finally, if the path does not exist and is not a dir then we'll try to download a file to that path
            {
                LOGDEBUG("Saving downloaded file to file: " << request.fileDownloadLocation.value());
                responseBuffer.downloadFilePath = request.fileDownloadLocation.value();
            }
            m_curlWrapper->curlEasySetFuncOpt(
                m_curlHandle, CURLOPT_WRITEFUNCTION, (void*)CurlFunctionsProvider::curlWriteFileFunc);
            m_curlWrapper->curlEasySetDataOpt(m_curlHandle, CURLOPT_WRITEDATA, &responseBuffer);
        }
        else
        {
            m_curlWrapper->curlEasySetFuncOpt(
                m_curlHandle, CURLOPT_WRITEFUNCTION, (void*)CurlFunctionsProvider::curlWriteFunc);
            m_curlWrapper->curlEasySetDataOpt(m_curlHandle, CURLOPT_WRITEDATA, &bodyBuffer);
        }

        // Handle URL parameters, such as: example.com?param1=value&param2=abc
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

        // Handle if the user wants to upload a file
        std::unique_ptr<FILE, int (*)(FILE*)> fileToSend(nullptr, fclose);
        if (request.fileToUpload.has_value())
        {
            if (FileSystem::fileSystem()->isFile(request.fileToUpload.value()))
            {
                m_curlWrapper->curlEasySetFuncOpt(
                    m_curlHandle, CURLOPT_READFUNCTION, (void*)CurlFunctionsProvider::curlFileReadFunc);
                fileToSend.reset(fopen(request.fileToUpload.value().c_str(), "rb"));
                if (fileToSend != nullptr)
                {
                    LOGDEBUG(
                        "Sending file: " << request.fileToUpload.value() << ", size: "
                                         << FileSystem::fileSystem()->fileSize(request.fileToUpload.value()));

                    m_curlWrapper->curlEasySetOpt(m_curlHandle, CURLOPT_UPLOAD, 1L);
                    m_curlWrapper->curlEasySetDataOpt(m_curlHandle, CURLOPT_READDATA, fileToSend.get());
                    m_curlWrapper->curlEasySetOpt(
                        m_curlHandle,
                        CURLOPT_INFILESIZE_LARGE,
                        (curl_off_t)FileSystem::fileSystem()->fileSize(request.fileToUpload.value()));
                    m_curlWrapper->curlEasySetFuncOpt(
                        m_curlHandle, CURLOPT_SEEKFUNCTION, (void*)CurlFunctionsProvider::curlSeekFileFunc);
                    m_curlWrapper->curlEasySetDataOpt(m_curlHandle, CURLOPT_SEEKDATA, fileToSend.get());
                }
                else
                {
                    response.error = "Failed to open file for reading: " + request.fileToUpload.value();
                    response.errorCode = HttpRequests::ResponseErrorCode::FAILED;
                    return response;
                }
            }
            else
            {
                response.error = "File to upload does not exist: " + request.fileToUpload.value();
                response.errorCode = HttpRequests::ResponseErrorCode::UPLOAD_FILE_DOES_NOT_EXIST;
                return response;
            }
        }

        // Set the request URL
        curlOptions.emplace_back("URL - CURLOPT_URL", CURLOPT_URL, request.url);

        // Set the preferred TLS version
        curlOptions.emplace_back("TLS/SSL version - CURLOPT_SSLVERSION", CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

        // Set timeout in seconds, defaulted to 10 mins in the request interface but can be set by user
        curlOptions.emplace_back("Set timeout - CURLOPT_TIMEOUT", CURLOPT_TIMEOUT, request.timeout);

        // cURL or openssl does not seem to support setting this protocol.
        // Could be worth an update, checking of build options and a rebuild of openssl and curl
        // curlOptions.emplace_back("Set HTTP version - CURLOPT_HTTP_VERSION", CURLOPT_HTTP_VERSION,
        // CURL_HTTP_VERSION_2TLS);

        // Handle setting the port
        if (request.port.has_value())
        {
            curlOptions.emplace_back("Port - CURLOPT_PORT", CURLOPT_PORT, request.port.value());
        }

        // Set the request type: GET, POST, PUT, DELETE, OPTIONS
        switch (request.requestType)
        {
            case Common::HttpRequests::GET:
                curlOptions.emplace_back("GET request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "GET");
                break;
            case Common::HttpRequests::POST:
                curlOptions.emplace_back("POST request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "POST");
                if (request.data.has_value())
                {
                    curlOptions.emplace_back(
                        "POST data - CURLOPT_COPYPOSTFIELDS", CURLOPT_COPYPOSTFIELDS, request.data.value());
                }
                break;
            case Common::HttpRequests::PUT:
                curlOptions.emplace_back("PUT request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "PUT");
                if (request.data.has_value())
                {
                    curlOptions.emplace_back(
                        "PUT data - CURLOPT_COPYPOSTFIELDS", CURLOPT_COPYPOSTFIELDS, request.data.value());
                }
                break;
            case Common::HttpRequests::DELETE:
                curlOptions.emplace_back("DELETE request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
            case Common::HttpRequests::OPTIONS:
                curlOptions.emplace_back("OPTIONS request - CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, "OPTIONS");
                break;
        }
        // Handle proxy being specified by user
        if (request.proxy.has_value())
        {
            curlOptions.emplace_back(
                "Set cURL to choose best authentication available - CURLOPT_PROXYAUTH",
                CURLOPT_PROXYAUTH,
                static_cast<long> CURLAUTH_ANY); // cast to long because the curl_easy_setopt function takes long but
                                                 // the constant is unsigned long
            curlOptions.emplace_back("Specify proxy - CURLOPT_PROXY", CURLOPT_PROXY, request.proxy.value());
            if (request.proxyUsername.has_value() && request.proxyPassword.has_value())
            {
                std::string encodedUsername = curlEscape(request.proxyUsername.value());
                std::string encodedPassword = curlEscape(request.proxyPassword.value());
                curlOptions.emplace_back(
                    "Set proxy user and password - CURLOPT_PROXYUSERPWD",
                    CURLOPT_PROXYUSERPWD,
                    encodedUsername + ":" + encodedPassword);
            }
        }
        else
        {
            if (request.allowEnvironmentProxy)
            {
                curlOptions.emplace_back(
                    "Set cURL to choose best authentication available - CURLOPT_PROXYAUTH",
                    CURLOPT_PROXYAUTH,
                    static_cast<long> CURLAUTH_ANY); // cast to long because the curl_easy_setopt function takes long
                                                     // but the constant is unsigned long
            }
            else
            {
                // Curl will use env proxies (which we should only ever read then set explicitly), not doing this causes
                // curl to use an env proxy when we explicitly ask to go directly
                curlOptions.emplace_back("Set no proxy - CURLOPT_NOPROXY", CURLOPT_NOPROXY, "*");
            }
        }

        // Handle allowing redirects, a lot of sites redirect, a user must explicitly ask for this to be enabled
        if (request.allowRedirects)
        {
            long followRedirects = 1;
            curlOptions.emplace_back(
                "Follow redirects - CURLOPT_FOLLOWLOCATION", CURLOPT_FOLLOWLOCATION, followRedirects);
            curlOptions.emplace_back("Set max redirects - CURLOPT_MAXREDIRS", CURLOPT_MAXREDIRS, 50);
        }

        // Handle a download speed / bandwidth limit being set by a user
        if (request.bandwidthLimit.has_value())
        {
            curlOptions.emplace_back(
                "Set max bandwidth - CURLOPT_MAX_RECV_SPEED_LARGE",
                CURLOPT_MAX_RECV_SPEED_LARGE,
                (curl_off_t)request.bandwidthLimit.value());
        }

        // Set logging verbosity if required
        if (m_curlVerboseLogging)
        {
            curlOptions.emplace_back("Enable verbose logging - CURLOPT_VERBOSE", CURLOPT_VERBOSE, 1L);
        }

        // Handle setting certs, either user specified or we try known system locations.
        if (request.certPath.has_value())
        {
            LOGDEBUG("Using client specified CA path: " << request.certPath.value());
            curlOptions.emplace_back("Path for CA bundle - CURLOPT_CAINFO", CURLOPT_CAINFO, request.certPath.value());
        }
        else
        {
            const std::vector<std::string> caPaths = { "/etc/ssl/certs/ca-certificates.crt",
                                                       "/etc/pki/tls/certs/ca-bundle.crt",
                                                       "/etc/ssl/ca-bundle.pem" };
            bool caPathFound = false;
            for (const auto& caPath : caPaths)
            {
                if (Common::FileSystem::fileSystem()->isFile(caPath))
                {
                    caPathFound = true;
                    LOGDEBUG("Using system CA path: " << caPath);
                    curlOptions.emplace_back("Path for CA bundle - CURLOPT_CAINFO", CURLOPT_CAINFO, caPath);
                    curlOptions.emplace_back(
                        "Path for CA dir - CURLOPT_CAPATH", CURLOPT_CAPATH, Common::FileSystem::dirName(caPath));
                    break;
                }
            }

            if (!caPathFound)
            {
                response.error = "No CA paths could be used";
                response.errorCode = HttpRequests::ResponseErrorCode::CERTIFICATE_ERROR;
                return response;
            }
        }

        // Handle headers, create a curl header list and add user headers to it
        struct curl_slist* curlHeaders = nullptr;
        if (request.headers.has_value())
        {
            for (auto const& [header, value] : request.headers.value())
            {
                LOGDEBUG("Append header: " << header << ": " << value);
                std::string h = header;
                h += ": ";
                h += value;
                curlHeaders = m_curlWrapper->curlSlistAppend(curlHeaders, h);
                if (!curlHeaders)
                {
                    m_curlWrapper->curlSlistFreeAll(curlHeaders);
                    LOGERROR("Failed to append header to request");
                    response.error = "Failed to append header: " + header;
                    response.errorCode = HttpRequests::ResponseErrorCode::FAILED;
                    return response;
                }
            }
        }
        Common::CurlWrapper::SListScopeGuard curlListScopeGuard(curlHeaders, *m_curlWrapper);

        // Apply all the cURL options set above
        for (const auto& curlOption : curlOptions)
        {
            LOGDEBUG("Setting cURL option: " << std::get<0>(curlOption));
            CURLcode result =
                m_curlWrapper->curlEasySetOpt(m_curlHandle, std::get<1>(curlOption), std::get<2>(curlOption));

            if (result != CURLE_OK)
            {
                LOGERROR("Failed to set curl option: " << std::get<0>(curlOption));
                response.error = m_curlWrapper->curlEasyStrError(result);
                response.errorCode = HttpRequests::ResponseErrorCode::FAILED;
                return response;
            }
        }

        // Apply the curl header list to our curl handle
        if (curlHeaders)
        {
            CURLcode result = m_curlWrapper->curlEasySetOptHeaders(m_curlHandle, curlHeaders);
            if (result != CURLE_OK)
            {
                LOGERROR("Failed to set headers");
                response.error = m_curlWrapper->curlEasyStrError(result);
                response.errorCode = HttpRequests::ResponseErrorCode::FAILED;
                return response;
            }
        }

        // Try to perform the request
        try
        {
            LOGDEBUG("Performing request: " << request.url);
            CURLcode result = m_curlWrapper->curlEasyPerform(m_curlHandle);
            LOGDEBUG("Performed request: " << result);
            if (result != CURLE_OK)
            {
                response.error = m_curlWrapper->curlEasyStrError(result);
                if (result == CURLE_OPERATION_TIMEDOUT)
                {
                    response.errorCode = HttpRequests::ResponseErrorCode::TIMEOUT;
                }
                else if (result == CURLE_SSL_CACERT_BADFILE || result == CURLE_SSL_CERTPROBLEM)
                {
                    response.errorCode = HttpRequests::ResponseErrorCode::CERTIFICATE_ERROR;
                }
                else if (result == CURLE_COULDNT_RESOLVE_HOST)
                {
                    response.errorCode = HttpRequests::ResponseErrorCode::COULD_NOT_RESOLVE_HOST;
                }
                else if (result == CURLE_COULDNT_RESOLVE_PROXY)
                {
                    response.errorCode = HttpRequests::ResponseErrorCode::COULD_NOT_RESOLVE_PROXY;
                    if (request.proxy.has_value())
                    {
                        response.error += " — " + request.proxy.value();
                    }
                    else
                    { // Should not be able to get here
                        response.error += ". Could not find proxy address used.";
                    }
                }
                else if (bodyBuffer.tooBig_)
                {
                    LOGERROR("HTTP Response too big");
                    response.errorCode = HttpRequests::ResponseErrorCode::REQUEST_FAILED;
                    response.error = "Response too big";
                }
                else
                {
                    response.errorCode = HttpRequests::ResponseErrorCode::REQUEST_FAILED;
                }
                return response;
            }
        }
        catch (const std::exception& ex)
        {
            response.error = "Request threw exception: " + std::string(ex.what());
            response.errorCode = HttpRequests::ResponseErrorCode::REQUEST_FAILED;
            return response;
        }

        // Store the body to be returned in the response, this will be empty if the user opted to download to a file
        assert(!bodyBuffer.tooBig_);
        response.body = bodyBuffer.buffer_;

        // Store any headers that were sent in the response
        response.headers = responseBuffer.headers;

        // Get the HTTP response code and add it to the response
        long responseCode = 0;
        if (m_curlWrapper->curlGetResponseCode(m_curlHandle, &responseCode) == CURLE_OK)
        {
            response.status = responseCode;
        }
        else
        {
            response.error = "Failed to get response code from cURL";
            response.errorCode = HttpRequests::ResponseErrorCode::FAILED;
            return response;
        }

        // If we're here then the request did not error, so set return code to be OK
        response.errorCode = HttpRequests::ResponseErrorCode::OK;
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

    std::string HttpRequesterImpl::curlEscape(const std::string& stringToEscape)
    {
        char* escaped = curl_easy_escape(m_curlHandle, stringToEscape.c_str(), stringToEscape.length());
        if (escaped)
        {
            std::string escapedString = std::string(escaped);
            curl_free(escaped);
            return escapedString;
        }
        throw Common::HttpRequests::HttpRequestsException("Failed to escape string: " + stringToEscape);
    }

    void HttpRequesterImpl::sendTerminate()
    {
        CurlFunctionsProvider::sendTerminateRequest();
    }
} // namespace Common::HttpRequestsImpl