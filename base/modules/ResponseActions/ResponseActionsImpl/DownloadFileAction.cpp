// Copyright 2023 Sophos Limited. All rights reserved.

#include "DownloadFileAction.h"

#include "ActionsUtils.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include <Common/FileSystem/IFileTooLargeException.h>
#include <Common/ProxyUtils/ProxyUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/ZipUtilities/ZipUtils.h>

//For return value interpretation
#include <minizip/mz_compat.h>

namespace ResponseActionsImpl
{
    DownloadFileAction::DownloadFileAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> client):
        m_client(std::move(client)){}

    std::string DownloadFileAction::run(const std::string& actionJson)
    {
        nlohmann::json response;
        response["type"] = "sophos.mgt.response.DownloadFile";
        DownloadInfo info;

        try
        {
            info = ActionsUtils::readDownloadAction(actionJson);
        }
        catch (const InvalidCommandFormat& exception)
        {
            ActionsUtils::setErrorInfo(response, 1,  "Error parsing command from Central");
            return response.dump();
        }

        if (!initialChecks(info, response))
        {
            return response.dump();
        }

        Common::UtilityImpl::FormattedTime time;
        u_int64_t start = time.currentEpochTimeInSecondsAsInteger();
        response["startedAt"] = start;

        download(info, response);

        if (response["httpStatus"] == Common::HttpRequests::HTTP_STATUS_OK)
        {
            assert(!response.contains("errorType"));
            assert(!response.contains("errorMessage"));

            std::string fileName = verifyFile(info, response);
            if (!fileName.empty())
            {
                decompressAndMoveFile(fileName, info, response);
            }
        }

        u_int64_t end = time.currentEpochTimeInSecondsAsInteger();
        response["duration"] = end - start;
        return response.dump();
    }

    bool DownloadFileAction::initialChecks(const DownloadInfo& info, nlohmann::json& response)
    {
        if (ActionsUtils::isExpired(info.expiration))
        {
            std::string error = "Download file action has expired";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response,4,error);
            return false;
        }

        const unsigned long maxFileSize = 1024UL * 1024 * 1024 * 2;

        if (info.sizeBytes > maxFileSize)
        {
            std::stringstream sizeError;
            sizeError << "Downloading file to " << info.targetPath << " failed due to size: " << info.sizeBytes << " is to large";
            LOGWARN(sizeError.str());
            ActionsUtils::setErrorInfo(response, 1, sizeError.str());
            return false;
        }

        if (info.targetPath.front() != '/')
        {
            std::string error = info.targetPath + " is not a valid absolute path";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response, 1, error, "invalid_path");
            return false;
        }

        if (m_fileSystem->exists(info.targetPath))
        {
            std::stringstream existsError;
            existsError << "Path " << info.targetPath << " already exists";
            LOGWARN(existsError.str());
            ActionsUtils::setErrorInfo(response, 1, existsError.str(), "path_exists");
            return false;
        }

        std::filesystem::space_info tmpSpaceInfo;
        std::filesystem::space_info destSpaceInfo;

        try
        {
            tmpSpaceInfo = m_fileSystem->getDiskSpaceInfo(m_raTmpDir);
            destSpaceInfo = m_fileSystem->getDiskSpaceInfo(findBaseDir(info.targetPath));
        }
        catch (const std::exception& e)
        {
            LOGERROR("Cant determine disk space on filesystem: " <<  e.what());
            return false;
        }
        std::filesystem::space_info spaceInfoToCheck = tmpSpaceInfo.available > destSpaceInfo.available ? destSpaceInfo : tmpSpaceInfo;

        if ((!info.decompress && spaceInfoToCheck.available < info.sizeBytes) ||
            (info.decompress && spaceInfoToCheck.available < (info.sizeBytes * 2.5)))
        {
            std::stringstream spaceError;
            spaceError << "Not enough space to complete download action : sophos install disk has "  << tmpSpaceInfo.available <<
                " ,destination disk has " << destSpaceInfo.available;
            LOGWARN(spaceError.str());
            ActionsUtils::setErrorInfo(response, 1, spaceError.str(), "not_enough_space");
            return false;
        }
        return true;
    }


    void DownloadFileAction::download(const DownloadInfo& info, nlohmann::json& response)
    {
        Common::HttpRequests::RequestConfig request{
            .url = info.url, .fileDownloadLocation = m_raTmpDir, .timeout = info.timeout
        };
        LOGINFO("Downloading to " << m_raTmpDir << " from url: " << info.url);
        Common::HttpRequests::Response httpresponse;

        if (Common::ProxyUtils::updateHttpRequestWithProxyInfo(request))
        {
            std::stringstream message;
            message << "Downloading via proxy: " << request.proxy.value();
            LOGINFO(message.str());
            httpresponse = m_client->get(request);
            handleHttpResponse(httpresponse, response, info.url);

            if (response["result"] != 0)
            {
                LOGWARN("Connection with proxy failed going direct");
                request.proxy = "";
                request.proxyPassword = "";
                request.proxyUsername = "";
                ActionsUtils::resetErrorInfo(response);
                httpresponse = m_client->get(request);
                handleHttpResponse(httpresponse, response, info.url);
            }
        }
        else
        {
            LOGINFO("Downloading directly");
            httpresponse = m_client->get(request);
            handleHttpResponse(httpresponse, response, info.url);
        }
        response["httpStatus"] = httpresponse.status;
    }

    void DownloadFileAction::removeTmpFiles()
    {
        m_fileSystem->removeFileOrDirectory(m_raTmpDir);
    }

    Path DownloadFileAction::verifyFile(const DownloadInfo& info, nlohmann::json& response)
    {
        //We are only downloading a single file and response actions should not run in parallel
        auto fileNameVec = m_fileSystem->listFiles(m_raTmpDir);
        assert(fileNameVec.size() == 1);
        Path filePath = fileNameVec.front();
        std::string fileName = Common::FileSystem::basename(filePath);
        LOGDEBUG("Downloaded file " << fileName);

        //Check sha256
        std::string fileSha = "";

        try
        {
            fileSha = m_fileSystem->calculateDigest(Common::SslImpl::Digest::sha256, filePath);
        }
        catch (const Common::FileSystem::IFileSystemException&)
        {
            std::string error = fileName + " cannot be accessed";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response, 1, error, "access_denied");
            return "";
        }
        catch (const std::exception& exception)
        {
            std::stringstream error;
            error << "Unknown error when calculating digest of " << fileName << ": " << exception.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return "";
        }

        assert(fileSha != "");

        if (fileSha != info.sha256)
        {
            std::stringstream shaError;
            shaError << "Calculated Sha256 (" << fileSha << ") doesnt match that of file downloaded (" << info.sha256 << ")";
            LOGWARN(shaError.str());
            ActionsUtils::setErrorInfo(response, 1, shaError.str(), "access_denied");
            return "";
        }
        return filePath;
    }


    void DownloadFileAction::decompressAndMoveFile(const Path& filePath, const DownloadInfo& info, nlohmann::json& response)
    {
        assert(!filePath.empty());
        //decompress
        if (info.decompress)
        {
            int ret;
            const Path tmpExtractPath = m_raTmpDir + "/extract";

            try
            {
                if (info.password.empty())
                {
                    ret = Common::ZipUtilities::zipUtils().unzip(filePath, tmpExtractPath);
                }
                else
                {
                    ret = Common::ZipUtilities::zipUtils().unzip(filePath, tmpExtractPath, true, info.password);
                }
            }
            catch (const std::runtime_error&)
            {
                // logging is done in the zip util
                ret = 1;
            }

            if (ret == UNZ_OK)
            {
                auto extractedFile = m_fileSystem->listFiles(tmpExtractPath);
                assert(extractedFile.size() == 1);
                m_fileSystem->makedirs(info.targetPath);
                m_fileSystem->moveFile(extractedFile.front(), info.targetPath);
                removeTmpFiles();
                auto extractedFileName = Common::FileSystem::basename(extractedFile.front());
                LOGINFO(info.targetPath << "/" << extractedFileName << " downloaded successfully");
            }
            else
            {
                std::stringstream error;
                error << "Error unzipping " << filePath << " due to ";

                if (ret == UNZ_BADPASSWORD)
                {
                    error << "bad password";
                    LOGWARN(error.str());
                    ActionsUtils::setErrorInfo(response, 1, error.str(), "invalid_password");
                }
                else if (ret == UNZ_BADZIPFILE)
                {
                    error << "bad archive";
                    LOGWARN(error.str());
                    ActionsUtils::setErrorInfo(response, 1, error.str(), "invalid_archive");
                }
                else
                {
                    error << "error no " << ret;
                    LOGWARN(error.str());
                    ActionsUtils::setErrorInfo(response, 3, error.str());
                }
            }
        }
        else
        {
            m_fileSystem->makedirs(info.targetPath);
            m_fileSystem->moveFile(filePath, info.targetPath);
            removeTmpFiles();
            auto zipFileName = Common::FileSystem::basename(filePath);
            LOGINFO(info.targetPath << "/" << zipFileName << " downloaded successfully");
        }
    }

    void DownloadFileAction::handleHttpResponse(
        const Common::HttpRequests::Response& httpresponse,
        nlohmann::json& response,
        const std::string& url)
    {

        if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT)
        {
            std::stringstream error;
            error << "Timeout downloading from " << url;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 2, error.str());
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::DOWNLOAD_TARGET_ALREADY_EXISTS)
        {
            std::stringstream error;
            error << "Download failed as target exists already: " + httpresponse.error;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str());
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            if (httpresponse.status == Common::HttpRequests::HTTP_STATUS_OK)
            {
                response["result"] = 0;
                //Log success when we have filename
            }
            else
            {
                std::stringstream error;
                error << "Failed to download " << url << ": Error code " << httpresponse.status;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
            }
        }
        else
        {
            std::stringstream error;
            error << "Failed to download " << url << ", error: " << httpresponse.error;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
        }
    }

    Path DownloadFileAction::findBaseDir(const Path& path)
    {
        auto pos = path.find('/', 1);
        if (pos == std::string::npos)
        {
            return path;
        }
        return path.substr(0, pos);
    }
}