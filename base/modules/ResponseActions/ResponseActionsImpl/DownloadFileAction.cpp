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
            LOGWARN(exception.what());
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

            if (verifyFile(info, response))
            {
                decompressAndMoveFile(info, response);
            }
            removeTmpFiles();
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
            sizeError << "Downloading file to " << info.targetPath << " failed due to size: " << info.sizeBytes << " is too large";
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
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            std::stringstream exception;
            exception << "Cant determine disk space on filesystem: " <<  e.what();
            LOGERROR(exception.str());
            ActionsUtils::setErrorInfo(response, 1, exception.str());
            return false;
        }
        catch (const std::exception& e)
        {
            std::stringstream error;
            error << "Unknown exception when calculating disk space on filesystem: " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return false;
        }

        auto spaceInfoToCheck = std::min(tmpSpaceInfo.available, destSpaceInfo.available);

        if ((!info.decompress && spaceInfoToCheck < info.sizeBytes) ||
            (info.decompress && spaceInfoToCheck < (info.sizeBytes * 2.5)))
        {
            std::stringstream spaceError;
            spaceError << "Not enough space to complete download action: Sophos install disk has "  << tmpSpaceInfo.available <<
                ", destination disk has " << destSpaceInfo.available;
            LOGWARN(spaceError.str());
            ActionsUtils::setErrorInfo(response, 1, spaceError.str(), "not_enough_space");
            return false;
        }
        return true;
    }


    void DownloadFileAction::download(const DownloadInfo& info, nlohmann::json& response)
    {
        Common::HttpRequests::RequestConfig request{
            .url = info.url, .fileDownloadLocation = m_tmpDownloadFile, .timeout = info.timeout
        };
        LOGINFO("Beginning download to " << info.targetPath);
        LOGDEBUG("Download URL is " <<  info.url);
        Common::HttpRequests::Response httpresponse;

        if (Common::ProxyUtils::updateHttpRequestWithProxyInfo(request))
        {
            LOGINFO("Downloading via proxy: " << request.proxy.value());
            httpresponse = m_client->get(request);
            handleHttpResponse(httpresponse, response);

            if (response["result"] != 0)
            {
                LOGWARN("Connection with proxy failed, going direct");
                request.proxy = "";
                request.proxyPassword = "";
                request.proxyUsername = "";
                ActionsUtils::resetErrorInfo(response);
                httpresponse = m_client->get(request);
                handleHttpResponse(httpresponse, response);
            }
        }
        else
        {
            LOGINFO("Downloading directly");
            httpresponse = m_client->get(request);
            handleHttpResponse(httpresponse, response);
        }
        response["httpStatus"] = httpresponse.status;
    }

    void DownloadFileAction::handleHttpResponse(
        const Common::HttpRequests::Response& httpresponse,
        nlohmann::json& response)
    {

        if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT)
        {
            std::stringstream error;
            error << "Download timed out";
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
                error << "Failed to download, Error code: " << httpresponse.status;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
            }
        }
        else
        {
            std::stringstream error;
            error << "Failed to download, Error: " << httpresponse.error;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
        }
    }

    bool DownloadFileAction::verifyFile(const DownloadInfo& info, nlohmann::json& response)
    {
        //We are only downloading a single file and response actions should not run in parallel
        auto fileNameVec = m_fileSystem->listFiles(m_raTmpDir);
        if (fileNameVec.empty())
        {
            std::string error = "Expected one file in " + m_raTmpDir + " but there are none";
            LOGERROR(error);
            ActionsUtils::setErrorInfo(response, 1, error);
            return false;
        }
        if (fileNameVec.size() > 1)
        {
            std::stringstream error;
            error << "Expected one file in " << m_raTmpDir << " but there are " << fileNameVec.size();
            LOGERROR(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return false;
        }
        std::string fileName = Common::FileSystem::basename(info.targetPath);
        LOGDEBUG("Downloaded file: " << fileName << " as " << m_tmpDownloadFile);

        //Check sha256
        std::string fileSha;

        try
        {
            fileSha = m_fileSystem->calculateDigest(Common::SslImpl::Digest::sha256, m_tmpDownloadFile);
        }
        catch (const Common::FileSystem::IFileSystemException&)
        {
            std::string error = fileName + " cannot be accessed";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response, 1, error, "access_denied");
            return false;
        }
        catch (const std::exception& exception)
        {
            std::stringstream error;
            error << "Unknown error when calculating digest of " << fileName << ": " << exception.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return false;
        }

        assert(!info.sha256.empty());
        assert(!fileSha.empty());

        if (fileSha != info.sha256)
        {
            std::stringstream shaError;
            shaError << "Calculated Sha256 (" << fileSha << ") doesnt match that of file downloaded (" << info.sha256 << ")";
            LOGWARN(shaError.str());
            ActionsUtils::setErrorInfo(response, 1, shaError.str(), "access_denied");
            return false;
        }
        return true;
    }


    void DownloadFileAction::decompressAndMoveFile(const DownloadInfo& info, nlohmann::json& response)
    {
        if (info.decompress)
        {
            int ret;

            if (!m_fileSystem->exists(m_tmpExtractPath))
            {
                try
                {
                    m_fileSystem->makedirs(m_tmpExtractPath);
                }
                catch (const Common::FileSystem::IFileSystemException& e)
                {
                    std::string error = "Unable to create path to extract file to: " + m_tmpExtractPath + ": " + e.what();
                    LOGWARN(error);
                    ActionsUtils::setErrorInfo(response, 1, error, "access_denied");
                    return;
                }
                catch (const std::exception& e)
                {
                    std::stringstream error;
                    error << "Unknown error creating path to extract file to: " + m_tmpExtractPath + ": " + e.what();
                    LOGWARN(error.str());
                    ActionsUtils::setErrorInfo(response, 1, error.str());
                    return;
                }
            }

            try
            {
                if (info.password.empty())
                {
                    ret = Common::ZipUtilities::zipUtils().unzip(m_tmpDownloadFile, m_tmpExtractPath);
                }
                else
                {
                    ret = Common::ZipUtilities::zipUtils().unzip(m_tmpDownloadFile, m_tmpExtractPath, true, info.password);
                }
            }
            catch (const std::runtime_error&)
            {
                // logging is done in the zip util
                ret = 1;
            }

            if (ret == UNZ_OK)
            {
                auto extractedFile = m_fileSystem->listAllFilesInDirectoryTree(m_tmpExtractPath);

                if (extractedFile.empty())
                {
                    std::string error = "Unzip successful but no file found in " + m_tmpExtractPath;
                    LOGWARN(error);
                    ActionsUtils::setErrorInfo(response, 1, error);
                }
                else if (extractedFile.size() > 1)
                {
                    std::string error = "Unzip successful but more than one file found in " + m_tmpExtractPath;
                    LOGWARN(error);
                    ActionsUtils::setErrorInfo(response, 1, error);
                }
                else
                {
                    makeDirAndMoveFile(response, info.targetPath, extractedFile.front());
                }

            }
            else
            {
                std::stringstream error;
                error << "Error unzipping " << m_tmpDownloadFile << " due to ";

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
            makeDirAndMoveFile(response, info.targetPath, m_tmpDownloadFile);
        }
    }

    void DownloadFileAction::makeDirAndMoveFile(nlohmann::json& response, Path destPath, const Path& filePathToMove)
    {
        auto fileName = Common::FileSystem::basename(destPath);
        auto dirName = Common::FileSystem::dirName(destPath);

        if (dirName.empty())
        {
            dirName = "/";
        }

        try
        {
            m_fileSystem->makedirs(dirName);
            m_fileSystem->moveFile(filePathToMove, destPath);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            std::stringstream error;
            error << "Unable to make directory " << destPath << " and move " << fileName << " to it: " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "access_denied");
            return;
        }
        catch (const std::exception& e)
        {
            std::stringstream error;
            error << "Unknown error when making directory " << destPath << " or moving file " << fileName << ": " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return;
        }

        LOGINFO(destPath << " downloaded successfully");
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

    void DownloadFileAction::removeTmpFiles()
    {
        if (m_fileSystem->exists(m_tmpDownloadFile))
        {
            m_fileSystem->removeFile(m_tmpDownloadFile);
        }
        if (m_fileSystem->exists(m_tmpExtractPath))
        {
            m_fileSystem->removeFileOrDirectory(m_tmpExtractPath);
        }
    }
}