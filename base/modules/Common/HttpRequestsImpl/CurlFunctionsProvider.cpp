// Copyright 2022-2024 Sophos Limited. All rights reserved.

#include "CurlFunctionsProvider.h"

#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/StringUtils.h"
#include <atomic>

std::atomic<bool> CurlFunctionsProvider::terminateRequested = false;
size_t CurlFunctionsProvider::curlWriteFunc(void* ptr, size_t size, size_t nmemb, void* voidBuffer)
{
    if (terminateRequested)
    {
        return CURL_WRITEFUNC_ERROR;
    }
    auto* buffer = reinterpret_cast<WriteBackBuffer*>(voidBuffer);
    size_t totalSizeBytes = size * nmemb;
    if (buffer->tooBig_ || (buffer->maxSize_ > 0 && totalSizeBytes + buffer->buffer_.size() > buffer->maxSize_))
    {
        // too much data
        LOGERROR("HTTP response too big, ignoring further data");
        buffer->tooBig_ = true;
        return CURL_WRITEFUNC_ERROR;
    }
    buffer->buffer_.append(static_cast<char*>(ptr), totalSizeBytes);
    return totalSizeBytes;
}

size_t CurlFunctionsProvider::curlWriteFileFunc(
    void* ptr,
    size_t size,
    size_t nmemb,
    Common::HttpRequestsImpl::ResponseBuffer* responseBuffer)
{
    if (terminateRequested)
    {
        return CURL_WRITEFUNC_ERROR;
    }

    if (responseBuffer->url.empty())
    {
        LOGERROR("Response buffer must contain the URL in case a file name needs to be generated from the URL");
        return 0;
    }

    size_t totalSizeBytes = size * nmemb;
    std::string content;
    content.append(static_cast<char*>(ptr), totalSizeBytes);
    auto fileSystem = Common::FileSystem::fileSystem();
    if (responseBuffer->downloadFilePath.empty())
    {
        if (responseBuffer->downloadDirectory.empty())
        {
            LOGERROR("Neither download target filename or directory specified");
            return 0;
        }

        // If no download filename specified then we need to work it out from headers or URL
        // First, try to work out filename from Content-disposition header
        // Example: Content-Disposition: attachment; filename="filename.jpg"
        std::string contentDispositionHeaderTag = "Content-Disposition";
        std::string filenameTag = "filename";
        if (responseBuffer->headers.count(contentDispositionHeaderTag) &&
            Common::UtilityImpl::StringUtils::isSubstring(
                responseBuffer->headers[contentDispositionHeaderTag], filenameTag))
        {
            std::vector<std::string> contentOptiopns = Common::UtilityImpl::StringUtils::splitString(
                responseBuffer->headers[contentDispositionHeaderTag], ";");
            for (const auto& option : contentOptiopns)
            {
                if (Common::UtilityImpl::StringUtils::isSubstring(option, "="))
                {
                    std::vector<std::string> optionAndValue =
                        Common::UtilityImpl::StringUtils::splitString(option, "=");
                    if (optionAndValue.size() == 2)
                    {
                        std::function isWhitespaceOrQuote = [](char c) { return std::isspace(c) || c == '"'; };
                        std::string cleanOption =
                            Common::UtilityImpl::StringUtils::trim(optionAndValue[0], isWhitespaceOrQuote);
                        std::string cleanValue =
                            Common::UtilityImpl::StringUtils::trim(optionAndValue[1], isWhitespaceOrQuote);
                        if (cleanOption == filenameTag && !cleanValue.empty())
                        {
                            responseBuffer->downloadFilePath =
                                Common::FileSystem::join(responseBuffer->downloadDirectory, cleanValue);
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            // Work out filename from URL
            responseBuffer->downloadFilePath = Common::FileSystem::join(
                responseBuffer->downloadDirectory, Common::FileSystem::basename(responseBuffer->url));
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
        std::stringstream errorMessage;
        errorMessage << "Failed to append data to file: " << responseBuffer->downloadFilePath
                     << ", error: " << ex.what();
        throw Common::HttpRequests::HttpRequestsException(errorMessage.str());
    }
    return totalSizeBytes;
}

size_t CurlFunctionsProvider::curlWriteHeadersFunc(
    void* ptr,
    size_t size,
    size_t nmemb,
    Common::HttpRequestsImpl::ResponseBuffer* responseBuffer)
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

int CurlFunctionsProvider::curlWriteDebugFunc(
    [[maybe_unused]] CURL* handle,
    curl_infotype type,
    char* data,
    size_t size,
    [[maybe_unused]] void* userp)
{
    std::string debugLine = "cURL ";
    bool shouldLog = false;
    switch (type)
    {
        case CURLINFO_TEXT:
            debugLine += "Info: ";
            shouldLog = true;
            break;
        case CURLINFO_HEADER_OUT:
            debugLine += "=> Send header: ";
            shouldLog = true;
            break;
        case CURLINFO_DATA_OUT:
            // This can be encrypted, so not logging it
            debugLine += "=> Send data.";
            break;
        case CURLINFO_SSL_DATA_OUT:
            // This can be quite noisy, so not logging it
            debugLine += "=> Send SSL data.";
            break;
        case CURLINFO_HEADER_IN:
            debugLine += "<= Recv header: ";
            shouldLog = true;
            break;
        case CURLINFO_DATA_IN:
            debugLine += "<= Recv data.";
            break;
        case CURLINFO_SSL_DATA_IN:
            // This can be quite noisy, so not logging it
            debugLine += "<= Recv SSL data.";
            break;
        default:
            LOGDEBUG("Unsupported cURL debug info type");
            return 0;
    }
    if (shouldLog)
    {
        debugLine.append(data, size);
    }
    LOGDEBUG(debugLine);
    return 0;
}

size_t CurlFunctionsProvider::curlFileReadFunc(char* ptr, size_t size, size_t nmemb, FILE* stream)
{
    if (terminateRequested)
    {
        return CURL_WRITEFUNC_ERROR;
    }
    size_t retCode = fread(ptr, size, nmemb, stream);
    return retCode;
}

int CurlFunctionsProvider::curlSeekFileFunc(void* userp, curl_off_t offset, int origin)
{
    if (terminateRequested)
    {
        return CURL_SEEKFUNC_CANTSEEK;
    }
    try
    {
        FILE* upload_fd = (FILE*)userp;
        fseek(upload_fd, offset, origin);
        return CURL_SEEKFUNC_OK;
    }
    catch (const std::exception& ex)
    {
        return CURL_SEEKFUNC_CANTSEEK;
    }
}

void CurlFunctionsProvider::sendTerminateRequest()
{
    LOGDEBUG("Sent terminate request to the curl command");
    terminateRequested = true;
}
