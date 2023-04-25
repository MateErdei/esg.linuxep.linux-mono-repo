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

namespace FileSystem = Common::FileSystem;

namespace ResponseActionsImpl
{
    DownloadFileAction::DownloadFileAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> client):
        m_client(std::move(client)){}

    nlohmann::json DownloadFileAction::run(const std::string& actionJson)
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
            return response;
        }

        if (!initialChecks(info, response))
        {
            return response;
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
        return response;
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

        std::filesystem::space_info tmpSpaceInfo;
        std::filesystem::space_info destSpaceInfo;

        try
        {
            tmpSpaceInfo = m_fileSystem->getDiskSpaceInfo(m_raTmpDir);
            destSpaceInfo = m_fileSystem->getDiskSpaceInfo(findBaseDir(info.targetPath));
        }
        catch (const FileSystem::IFileSystemException& e)
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
            ActionsUtils::setErrorInfo(response, 2, error.str(), "network_error");
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
        std::string fileName = FileSystem::basename(info.targetPath);
        LOGDEBUG("Downloaded file: " << fileNameVec.front());
        assert(fileNameVec.front() == m_tmpDownloadFile);

        //Check sha256
        std::string fileSha;

        try
        {
            fileSha = m_fileSystem->calculateDigest(Common::SslImpl::Digest::sha256, m_tmpDownloadFile);
        }
        catch (const FileSystem::IFileSystemException&)
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
        LOGDEBUG("Successfully matched sha256 to downloaded file");
        return true;
    }


    void DownloadFileAction::decompressAndMoveFile(const DownloadInfo& info, nlohmann::json& response)
    {
        if (info.decompress)
        {
            int ret;

            if (!createExtractionDirectory(response))
            {
                return;
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
                auto extractedFiles = m_fileSystem->listAllFilesInDirectoryTree(m_tmpExtractPath);
                int noFiles = extractedFiles.size();
                if (noFiles == 0)
                {
                    std::string error = "Unzip successful but no file found in " + m_tmpExtractPath;
                    LOGWARN(error);
                    ActionsUtils::setErrorInfo(response, 1, error);
                }
                else
                {
                    LOGINFO("Extracted " << noFiles << " files from archive");

                    auto destDir = info.targetPath;
                    if (destDir.back() != '/')
                    {
                        destDir = (FileSystem::dirName(destDir) + "/");
                    }

                    if (makeDestDirectory(response, destDir))
                    {
                        if (noFiles == 1)
                        {
                            handleMovingSingleExtractedFile(response, destDir, info.targetPath, extractedFiles.front());
                        }
                        else
                        {
                            handleMovingMultipleExtractedFile(response, destDir, info.targetPath, extractedFiles);
                        }
                    }
                }
            }
            else
            {
                handleUnZipFailure(response, ret);
            }
        }
        else
        {
            handleMovingArchive(response, info.targetPath);
        }
    }

    bool DownloadFileAction::createExtractionDirectory(nlohmann::json& response)
    {
        if (!m_fileSystem->exists(m_tmpExtractPath))
        {
            try
            {
                m_fileSystem->makedirs(m_tmpExtractPath);
            }
            catch (const FileSystem::IFileSystemException& e)
            {
                std::string error = "Unable to create path to extract file to: " + m_tmpExtractPath + ": " + e.what();
                LOGWARN(error);
                ActionsUtils::setErrorInfo(response, 1, error, "access_denied");
                return false;
            }
            catch (const std::exception& e)
            {
                std::stringstream error;
                error << "Unknown error creating path to extract file to: " + m_tmpExtractPath + ": " + e.what();
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(response, 1, error.str());
                return false;
            }
        }
        return true;
    }

    void DownloadFileAction::handleMovingArchive(nlohmann::json& response, const Path& targetPath)
    {
        auto destDir = targetPath;
        Path fileName = "";
        if (destDir.back() != '/')
        {
            destDir = FileSystem::dirName(destDir) + "/";
            fileName = FileSystem::basename(targetPath);
        }
        else
        {
            fileName = m_archiveFileName;
        }

        if (makeDestDirectory(response, destDir) &&
            !fileAlreadyExists(response, destDir + fileName))
        {
            moveFile(response, destDir, fileName, m_tmpDownloadFile);
        }
    }

    void DownloadFileAction::handleMovingSingleExtractedFile(nlohmann::json& response, const Path& destDir, const Path& targetPath, const Path& extractedFile)
    {
        Path fileName = "";
        if (destDir.back() != '/')
        {
            fileName = FileSystem::basename(targetPath);
        }
        else
        {
            fileName = FileSystem::basename(extractedFile);
        }
        if (!fileAlreadyExists(response, destDir + fileName))
        {
            moveFile(response, destDir, fileName, extractedFile);
        }
    }

    void DownloadFileAction::handleMovingMultipleExtractedFile(nlohmann::json& response, const Path& destDir, const Path& targetPath, const std::vector<std::string>& extractedFiles)
    {
        //Dont use filename for multiple files
        if (targetPath.back() != '/')
        {
            std::string msg = "Ignoring filepath in targetPath field as the archive contains multiple files";
            LOGINFO("Ignoring filepath in targetPath field as the archive contains multiple files");
            ActionsUtils::setErrorInfo(response, 0, msg);
        }

        bool anyFileExists = false;
        //We need to fail entirely if any one file already exists
        for (const auto& filePath : extractedFiles)
        {
            auto fileName = FileSystem::basename(filePath);
            if (fileAlreadyExists(response, destDir + fileName))
            {
                anyFileExists = true;
                if (anyFileExists)
                {
                    //A warning in the response is added in fileAlreadyExists
                    LOGWARN("A file in the extracted archive already existed on destination path, aborting");
                    break;
                }
            }
        }

        if (!anyFileExists)
        {
            for (const auto& filePath : extractedFiles)
            {
                auto fileName = FileSystem::basename(filePath);
                moveFile(response, destDir, fileName, filePath);
            }
        }
    }

    void DownloadFileAction::handleUnZipFailure(nlohmann::json& response, const int& unzipReturn)
    {
        assert(unzipReturn != 0);

        std::stringstream error;
        error << "Error unzipping " << m_tmpDownloadFile << " due to ";

        if (unzipReturn == UNZ_BADPASSWORD)
        {
            error << "bad password";
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "invalid_password");
        }
        else if (unzipReturn == UNZ_BADZIPFILE)
        {
            error << "bad archive";
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "invalid_archive");
        }
        else
        {
            error << "error no " << unzipReturn;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 3, error.str());
        }
    }

    bool DownloadFileAction::fileAlreadyExists(nlohmann::json& response, const Path& destPath)
    {
        if (m_fileSystem->exists(destPath))
        {
            std::stringstream existsError;
            existsError << "Path " << destPath << " already exists";
            LOGWARN(existsError.str());
            ActionsUtils::setErrorInfo(response, 1, existsError.str(), "path_exists");
            return true;
        }
        return false;
    }

    bool DownloadFileAction::makeDestDirectory(nlohmann::json& response, const Path& destDir)
    {
        if (m_fileSystem->exists(destDir))
        {
            return true;
        }

        try
        {
            m_fileSystem->makedirs(destDir);
        }
        catch (const FileSystem::IFileSystemException& e)
        {
            std::stringstream error;
            error << "Unable to make directory " << destDir << " : " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "access_denied");
            return false;
        }
        catch (const std::exception& e)
        {
            std::stringstream error;
            error << "Unknown error when making directory " << destDir << " : " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return false;
        }
        return true;
    }

    void DownloadFileAction::moveFile(nlohmann::json& response, const Path& destDir, const Path& fileName, const Path& filePathToMove)
    {
        assert(destDir.back() == '/');
        auto destPath = destDir + fileName;

        try
        {
            m_fileSystem->moveFileTryCopy(filePathToMove, destPath);
        }
        catch (const FileSystem::IFileSystemException& e)
        {
            std::stringstream error;
            error << "Unable to move " << filePathToMove << " to " << destPath << ": " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "access_denied");
            return;
        }
        catch (const std::exception& e)
        {
            std::stringstream error;
            error << "Unknown error when moving file " << filePathToMove << " to " << destPath << ": " << e.what();
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