// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "DownloadFileAction.h"

#include "ActionsUtils.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/ProxyUtils/ProxyUtils.h"
#include "Common/Threads/NotifyPipe.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/ZipUtilities/ZipUtils.h"

// For return value interpretation
#include <minizip/mz_compat.h>
#include <future>
namespace FileSystem = Common::FileSystem;

namespace ResponseActionsImpl
{
    DownloadFileAction::DownloadFileAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> client,
                                           Common::ISignalHandlerSharedPtr sigHandler,
                                           Common::SystemCallWrapper::ISystemCallWrapperSharedPtr systemCallWrapper) :
            m_client(std::move(client)),
            m_SignalHandler(std::move(sigHandler)),
            m_SysCallWrapper(std::move(systemCallWrapper))
    {
    }

    nlohmann::json DownloadFileAction::run(const std::string& actionJson)
    {
        removeTmpFiles();

        m_response.clear();
        m_response["type"] = "sophos.mgt.response.DownloadFile";
        DownloadInfo info;

        try
        {
            info = ActionsUtils::readDownloadAction(actionJson);
        }
        catch (const InvalidCommandFormat& exception)
        {
            LOGWARN(exception.what());
            ActionsUtils::setErrorInfo(m_response, 1, "Error parsing command from Central");
            return m_response;
        }

        if (!initialChecks(info))
        {
            return m_response;
        }

        Common::UtilityImpl::FormattedTime time;
        u_int64_t start = time.currentEpochTimeInSecondsAsInteger();
        m_response["startedAt"] = start;

        download(info);

        if (m_response["httpStatus"] == Common::HttpRequests::HTTP_STATUS_OK)
        {
            assert(!m_response.contains("errorType"));
            assert(!m_response.contains("errorMessage"));

            if (verifyFile(info))
            {
                decompressAndMoveFile(info);
            }
        }

        removeTmpFiles();

        u_int64_t end = time.currentEpochTimeInSecondsAsInteger();
        m_response["duration"] = end - start;
        return m_response;
    }

    bool DownloadFileAction::initialChecks(const DownloadInfo& info)
    {
        if (ActionsUtils::isExpired(info.expiration))
        {
            std::string error = "Download file action has expired";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(m_response, 4, error);
            return false;
        }

        const unsigned long maxFileSize = 1024UL * 1024 * 1024 * 2;

        if (info.sizeBytes > maxFileSize)
        {
            std::stringstream sizeError;
            sizeError << "Downloading file to " << info.targetPath << " failed due to size: " << info.sizeBytes
                      << " is too large";
            LOGWARN(sizeError.str());
            ActionsUtils::setErrorInfo(m_response, 1, sizeError.str());
            return false;
        }

        if (info.targetPath.front() != '/')
        {
            std::string error = info.targetPath + " is not a valid absolute path";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(m_response, 1, error, "invalid_path");
            return false;
        }

        return assessSpaceInfo(info);
    }

    bool DownloadFileAction::assessSpaceInfo(const DownloadInfo& info)
    {
        std::filesystem::space_info tmpSpaceInfo;
        std::filesystem::space_info destSpaceInfo;

        try
        {
            std::error_code errCode;
            LOGINFO("Getting space info for " << m_raTmpDir);
            tmpSpaceInfo = m_fileSystem->getDiskSpaceInfo(findBaseDir(m_raTmpDir), errCode);
            if (errCode)
            {
                LOGERROR(
                    "Error calculating disk space for " << m_raTmpDir << ": " << errCode.value()
                                                        << " message:" << errCode.message());
                ActionsUtils::setErrorInfo(m_response, 1, errCode.message());
                return false;
            }

            errCode.clear();
            LOGINFO("Getting space info for " << info.targetPath);
            destSpaceInfo = m_fileSystem->getDiskSpaceInfo(findBaseDir(info.targetPath), errCode);
            if (errCode)
            {
                LOGERROR(
                    "Error calculating disk space for " << info.targetPath << ": " << errCode.value()
                                                        << " message:" << errCode.message());
                ActionsUtils::setErrorInfo(m_response, 1, errCode.message());
                return false;
            }
        }
        catch (const FileSystem::IFileSystemException& e)
        {
            std::stringstream exception;
            exception << "Cant determine disk space on filesystem: " << e.what();
            LOGERROR(exception.str());
            ActionsUtils::setErrorInfo(m_response, 1, exception.str());
            return false;
        }
        catch (const std::exception& e)
        {
            std::stringstream error;
            error << "Unknown exception when calculating disk space on filesystem: " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str());
            return false;
        }

        auto spaceInfoToCheck = std::min(tmpSpaceInfo.available, destSpaceInfo.available);

        if ((!info.decompress && spaceInfoToCheck < info.sizeBytes) ||
            (info.decompress && spaceInfoToCheck < (info.sizeBytes * 2.5)))
        {
            std::stringstream spaceError;
            spaceError << "Not enough space to complete download action: Sophos install disk has "
                       << tmpSpaceInfo.available << ", destination disk has " << destSpaceInfo.available;
            LOGWARN(spaceError.str());
            ActionsUtils::setErrorInfo(m_response, 1, spaceError.str(), "not_enough_space");
            return false;
        }
        return true;
    }

    void DownloadFileAction::download(const DownloadInfo& info)
    {
        Common::HttpRequests::RequestConfig request{ .url = info.url,
                                                     .fileDownloadLocation = m_tmpDownloadFile,
                                                     .timeout = info.timeout };
        LOGINFO("Beginning download to " << info.targetPath);
        LOGDEBUG("Download URL is " << info.url);
        Common::HttpRequests::Response httpresponse;

        if (Common::ProxyUtils::updateHttpRequestWithProxyInfo(request))
        {
            LOGINFO("Downloading via proxy: " << request.proxy.value());
            httpresponse = doGetRequest(request);
            handleHttpResponse(httpresponse);
            if (m_terminate || m_timeout)
            {
                return;
            }
            if (m_response["result"] != 0)
            {
                LOGWARN("Connection with proxy failed, going direct");
                request.proxy = "";
                request.proxyPassword = "";
                request.proxyUsername = "";
                ActionsUtils::resetErrorInfo(m_response);
                httpresponse = doGetRequest(request);
                handleHttpResponse(httpresponse);
            }
        }
        else
        {
            LOGINFO("Downloading directly");
            httpresponse = doGetRequest(request);
            handleHttpResponse(httpresponse);
        }
        m_response["httpStatus"] = httpresponse.status;
    }

    Common::HttpRequests::Response DownloadFileAction::doGetRequest(Common::HttpRequests::RequestConfig request)
    {
        std::unique_ptr<Common::Threads::NotifyPipe> notifyPipe = std::make_unique<Common::Threads::NotifyPipe>();
        auto fut = std::async( std::launch::async, [this,request,&notifyPipe]()
                               {
                                   Common::HttpRequests::Response response = m_client->get(request);
                                   notifyPipe->notify();
                                   return  response;
                               }
        );

        struct pollfd fds[]{ { .fd = m_SignalHandler->terminationFileDescriptor(), .events = POLLIN, .revents = 0 },
                             { .fd = m_SignalHandler->usr1FileDescriptor(), .events = POLLIN, .revents = 0 } ,
                                { .fd = notifyPipe->readFd(), .events = POLLIN, .revents = 0 }};
        while (true)
        {
            auto ret = m_SysCallWrapper->ppoll(fds, std::size(fds), nullptr, nullptr);

            if (ret < 0)
            {
                int err = errno;
                if (err == EINTR)
                {
                    LOGDEBUG("Ignoring EINTR from ppoll");
                    continue;
                }
                LOGERROR(
                        "Error from ppoll while waiting for command to finish: " << std::strerror(err) << "(" << err << ")");
                break;
            }
            else
            {
                if ((fds[0].revents & POLLIN) != 0)
                {
                    LOGINFO("RunDownloadFileAction has received termination command");
                    m_terminate = true;
                    m_client->sendTerminate();
                    break;
                }
                if ((fds[1].revents & POLLIN) != 0)
                {
                    LOGINFO("RunDownloadFileAction has received termination command due to timeout");
                    m_timeout = true;
                    m_client->sendTerminate();
                    break;
                }
                if ((fds[2].revents & POLLIN) != 0)
                {
                    LOGDEBUG("Curl request finished");
                    break;
                }
            }

        }
        return fut.get();
    }

    void DownloadFileAction::handleHttpResponse(const Common::HttpRequests::Response& httpresponse)
    {
        if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT)
        {
            std::stringstream error;
            error << "Download timed out";
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 2, error.str(), "network_error");
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::DOWNLOAD_TARGET_ALREADY_EXISTS)
        {
            std::stringstream error;
            error << "Download failed as target exists already: " + httpresponse.error;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str());
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            if (httpresponse.status == Common::HttpRequests::HTTP_STATUS_OK)
            {
                m_response["result"] = 0;
                // Log success when we have filename
            }
            else
            {
                std::stringstream error;
                error << "Failed to download, Error code: " << httpresponse.status;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(m_response, 1, error.str(), "network_error");
            }
        }
        else
        {
            std::stringstream error;
            error << "Failed to download, Error: " << httpresponse.error;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str(), "network_error");
        }
        if (m_timeout)
        {
            m_response["result"] = ResponseActions::RACommon::ResponseResult::TIMEOUT;
            return;
        }
    }

    bool DownloadFileAction::verifyFile(const DownloadInfo& info)
    {
        // We are only downloading a single file and response actions should not run in parallel
        auto fileNameVec = m_fileSystem->listFiles(m_raTmpDir);
        if (fileNameVec.empty())
        {
            std::string error = "Expected one file in " + m_raTmpDir + " but there are none";
            LOGERROR(error);
            ActionsUtils::setErrorInfo(m_response, 1, error);
            return false;
        }
        if (fileNameVec.size() > 1)
        {
            std::stringstream error;
            error << "Expected one file in " << m_raTmpDir << " but there are " << fileNameVec.size();
            LOGERROR(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str());
            return false;
        }
        std::string fileName = FileSystem::basename(info.targetPath);
        LOGDEBUG("Downloaded file: " << fileNameVec.front());
        assert(fileNameVec.front() == m_tmpDownloadFile);

        // Check sha256
        std::string fileSha;

        try
        {
            fileSha = m_fileSystem->calculateDigest(Common::SslImpl::Digest::sha256, m_tmpDownloadFile);
        }
        catch (const FileSystem::IFileSystemException&)
        {
            std::string error = fileName + " cannot be accessed";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(m_response, 1, error, "access_denied");
            return false;
        }
        catch (const std::exception& exception)
        {
            std::stringstream error;
            error << "Unknown error when calculating digest of " << fileName << ": " << exception.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str());
            return false;
        }

        assert(!info.sha256.empty());
        assert(!fileSha.empty());

        if (fileSha != info.sha256)
        {
            std::stringstream shaError;
            shaError << "Calculated Sha256 (" << fileSha << ") doesnt match that of file downloaded (" << info.sha256
                     << ")";
            LOGWARN(shaError.str());
            ActionsUtils::setErrorInfo(m_response, 1, shaError.str(), "access_denied");
            return false;
        }
        LOGDEBUG("Successfully matched sha256 to downloaded file");
        return true;
    }

    void DownloadFileAction::decompressAndMoveFile(const DownloadInfo& info)
    {
        if (info.decompress)
        {
            int ret;

            if (!createExtractionDirectory())
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
                    ret = Common::ZipUtilities::zipUtils().unzip(
                        m_tmpDownloadFile, m_tmpExtractPath, true, info.password);
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
                    ActionsUtils::setErrorInfo(m_response, 1, error);
                }
                else
                {
                    std::string files = noFiles > 1 ? "files" : "file";
                    LOGINFO("Extracted " << noFiles << " " << files << " from archive");

                    auto destDir = info.targetPath;
                    if (destDir.back() != '/')
                    {
                        destDir = (FileSystem::dirName(destDir) + "/");
                    }

                    if (makeDestDirectory(destDir))
                    {
                        if (noFiles == 1)
                        {
                            handleMovingSingleExtractedFile(destDir, info.targetPath, extractedFiles.front());
                        }
                        else
                        {
                            handleMovingMultipleExtractedFile(destDir, info.targetPath, extractedFiles);
                        }
                    }
                }
            }
            else
            {
                handleUnZipFailure(ret);
            }
        }
        else
        {
            handleMovingArchive(info.targetPath);
        }
    }

    bool DownloadFileAction::createExtractionDirectory()
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
                ActionsUtils::setErrorInfo(m_response, 1, error, "access_denied");
                return false;
            }
            catch (const std::exception& e)
            {
                std::stringstream error;
                error << "Unknown error creating path to extract file to: " + m_tmpExtractPath + ": " + e.what();
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(m_response, 1, error.str());
                return false;
            }
        }
        return true;
    }

    void DownloadFileAction::handleMovingArchive(const Path& targetPath)
    {
        auto destDir = targetPath;
        Path fileName;
        if (destDir.back() != '/')
        {
            destDir = FileSystem::dirName(destDir) + "/";
            fileName = FileSystem::basename(targetPath);
        }
        else
        {
            fileName = m_archiveFileName;
        }

        if (makeDestDirectory(destDir) && !fileAlreadyExists(destDir + fileName))
        {
            moveFile(destDir, fileName, m_tmpDownloadFile);
        }
    }

    void DownloadFileAction::handleMovingSingleExtractedFile(
        const Path& destDir,
        const Path& targetPath,
        const Path& extractedFile)
    {
        Path subFilePath;
        if (destDir.back() != '/')
        {
            subFilePath = targetPath;
        }
        else
        {
            subFilePath = extractedFile;
        }

        subFilePath = getSubDirsInTmpDir(subFilePath);

        if (!fileAlreadyExists(destDir + subFilePath))
        {
            moveFile(destDir, FileSystem::basename(subFilePath), extractedFile);
        }
    }

    void DownloadFileAction::handleMovingMultipleExtractedFile(
        const Path& destDir,
        const Path& targetPath,
        const std::vector<std::string>& extractedFiles)
    {
        // Dont use filename for multiple files
        if (targetPath.back() != '/')
        {
            std::string msg = "Ignoring filepath in targetPath field as the archive contains multiple files";
            LOGDEBUG(msg);
            ActionsUtils::setErrorInfo(m_response, 0, msg);
        }

        bool anyFileExists = false;
        // We need to fail entirely if any one file already exists
        for (const auto& filePath : extractedFiles)
        {
            Path subFilePath = getSubDirsInTmpDir(filePath);

            if (fileAlreadyExists(destDir + subFilePath))
            {
                anyFileExists = true;
                // A warning in the response is added in fileAlreadyExists
                LOGWARN("A file in the extracted archive already existed on destination path, aborting");
                break;
            }
        }

        if (!anyFileExists)
        {
            bool success = true;

            for (const auto& filePath : extractedFiles)
            {
                auto fileName = FileSystem::basename(filePath);
                if (!moveFile(destDir, fileName, filePath))
                {
                    success = false;
                    break;
                }
            }

            if (!success)
            {
                for (const auto& filePath : extractedFiles)
                {
                    Path pathToRemove = destDir + getSubDirsInTmpDir(filePath);

                    LOGDEBUG("Removing file " << pathToRemove << " as move file failed");
                    removeDestDir(pathToRemove);
                }
            }
        }
    }

    void DownloadFileAction::handleUnZipFailure(const int& unzipReturn)
    {
        assert(unzipReturn != 0);

        std::stringstream error;
        error << "Error unzipping " << m_tmpDownloadFile << " due to ";

        if (unzipReturn == UNZ_BADPASSWORD)
        {
            error << "bad password";
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str(), "invalid_password");
        }
        else if (unzipReturn == UNZ_BADZIPFILE)
        {
            error << "invalid zip file";
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str(), "invalid_archive");
        }
        else
        {
            error << "error no " << unzipReturn;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 3, error.str());
        }
    }

    bool DownloadFileAction::fileAlreadyExists(const Path& destPath)
    {
        if (m_fileSystem->exists(destPath))
        {
            std::stringstream existsError;
            existsError << "Path " << destPath << " already exists";
            LOGWARN(existsError.str());
            ActionsUtils::setErrorInfo(m_response, 1, existsError.str(), "path_exists");
            return true;
        }
        return false;
    }

    bool DownloadFileAction::makeDestDirectory(const Path& destDir)
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
            ActionsUtils::setErrorInfo(m_response, 1, error.str(), "access_denied");
            return false;
        }
        catch (const std::exception& e)
        {
            std::stringstream error;
            error << "Unknown error when making directory " << destDir << " : " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str());
            return false;
        }
        return true;
    }

    bool DownloadFileAction::moveFile(const Path& destDir, const Path& fileName, const Path& filePathToMove)
    {
        assert(destDir.back() == '/');
        auto destPath = destDir + fileName;
        Path dirName = FileSystem::dirName(filePathToMove);

        try
        {
            dirName = getSubDirsInTmpDir(dirName);

            if (!dirName.empty())
            {
                m_fileSystem->makedirs(destDir + dirName);
                destPath = destDir + dirName + "/" + fileName;
            }
        }
        catch (const FileSystem::IFileSystemException& e)
        {
            std::stringstream error;
            error << "Unable to make directories " << destDir + dirName << ": " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str());
            return false;
        }
        catch (const std::exception& e)
        {
            std::stringstream error;
            error << "Unknown error when trying to make directories " << destDir + dirName << ": " << e.what();
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(m_response, 1, error.str());
            return false;
        }

        try
        {
            m_fileSystem->moveFileTryCopy(filePathToMove, destPath);
        }
        catch (const FileSystem::IFileSystemException& e)
        {
            std::stringstream error;
            LOGWARN(e.what());
            auto msg = removeDestDir(destPath) ? "not_enough_space" : "access_denied";
            ActionsUtils::setErrorInfo(m_response, 1, e.what(), msg);
            return false;
        }
        catch (const std::exception& e)
        {
            std::stringstream error;
            error << "Unknown error when moving file " << filePathToMove << " to " << destPath << ": " << e.what();
            LOGWARN(error.str());
            removeDestDir(destPath);
            ActionsUtils::setErrorInfo(m_response, 1, error.str());
            return false;
        }

        LOGINFO(destPath << " downloaded successfully");

        return true;
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

    bool DownloadFileAction::removeDestDir(const std::string& destPath)
    {
        bool existed = false;
        if (m_fileSystem->exists(destPath))
        {
            existed = true;
            try
            {
                m_fileSystem->removeFileOrDirectory(destPath);
            }
            catch (const std::exception& ex)
            {
                LOGERROR("Failed to delete file at " << destPath);
            }
        }
        return existed;
    }

    // Determine whether the filePath to move is in an extract directory or not
    Path DownloadFileAction::getSubDirsInTmpDir(const Path& filePath)
    {
        Path subFilePath = filePath;
        if (filePath.rfind(m_tmpExtractPath, 0) == 0)
        {
            subFilePath.erase(0, m_tmpExtractPath.size() + 1);
        }
        else
        {
            subFilePath.erase(0, m_raTmpDir.size());
        }
        return subFilePath;
    }
} // namespace ResponseActionsImpl