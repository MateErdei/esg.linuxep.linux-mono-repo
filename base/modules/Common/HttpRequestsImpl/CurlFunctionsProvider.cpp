/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "CurlFunctionsProvider.h"

#include "Logger.h"

#include "FileSystem/IFileSystem.h"
#include "FileSystem/IFileSystemException.h"
#include "UtilityImpl/StringUtils.h"

size_t CurlFunctionsProvider::curlWriteFunc(void* ptr, size_t size, size_t nmemb, std::string* buffer)
{
    size_t totalSizeBytes = size * nmemb;
    buffer->append(static_cast<char*>(ptr), totalSizeBytes);
    return totalSizeBytes;
}

size_t CurlFunctionsProvider::curlWriteFileFunc(
    void* ptr,
    size_t size,
    size_t nmemb,
    Common::HttpRequestsImpl::ResponseBuffer* responseBuffer)
{
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
    bool should_log = false;
    switch (type)
    {
        case CURLINFO_TEXT:
            debugLine += "Info: ";
            should_log = true;
            break;
        case CURLINFO_HEADER_OUT:
            debugLine += "=> Send header: ";
            should_log = true;
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
            should_log = true;
            break;
        case CURLINFO_DATA_IN:
            debugLine += "<= Recv data: ";
            should_log = true;
            break;
        case CURLINFO_SSL_DATA_IN:
            // This can be quite noisy, so not logging it
            debugLine += "<= Recv SSL data.";
            break;
        default:
            LOGDEBUG("Unsupported cURL debug info type");
            return 0;
    }
    if (should_log)
    {
        debugLine.append(data, size);
    }
    LOGDEBUG(debugLine);
    return 0;
}

size_t CurlFunctionsProvider::curlFileReadFunc(char* ptr, size_t size, size_t nmemb, FILE* stream)
{
    size_t retCode = fread(ptr, size, nmemb, stream);
    return retCode;
}

int CurlFunctionsProvider::curlSeekFileFunc(void* userp, curl_off_t offset, int origin)
{
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
