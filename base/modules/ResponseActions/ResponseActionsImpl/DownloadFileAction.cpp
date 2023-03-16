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
            ActionsUtils::setErrorInfo(response, 1, "Error parsing command from Central");
            return response.dump();
        }

        if (ActionsUtils::isExpired(info.expiration))
        {
            std::string error = "Download file action has expired";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response,4,error);
            return response.dump();
        }

        if (!m_fileSystem->isDirectory(info.targetPath))
        {
            std::string error = info.targetPath + " is not a valid directory";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response, 1, error, "invalid_path");
            return response.dump();
        }

        Common::UtilityImpl::FormattedTime time;
        u_int64_t start = time.currentEpochTimeInSecondsAsInteger();
        response["startedAt"] = start;

        prepareAndDownload(info, response);

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
        //todo
      /*  if (info.compress)
        {
            if (fs->isFile(m_pathToDownload))
            {
                fs->removeFile(m_pathToDownload);
            }
        }*/

        u_int64_t end = time.currentEpochTimeInSecondsAsInteger();
        response["duration"] = end - start;
        return response.dump();
    }

    void DownloadFileAction::prepareAndDownload(const DownloadInfo& info, nlohmann::json& response)
    {
        const unsigned long maxFileSize = 1024UL * 1024 * 1024 * 2;

        if (info.sizeBytes > maxFileSize)
        {
            LOGERROR("Download file " << info.targetPath << " of size " << info.sizeBytes << " is to large");
            return;
        }

        if (m_fileSystem->exists(info.targetPath))
        {
            std::stringstream existsError;
            existsError << "Path " << info.targetPath << " already exists";
            LOGWARN(existsError.str());
            ActionsUtils::setErrorInfo(response, 1, existsError.str(), "path_exists");
            return;
        }

        const auto tmpSpaceInfo = m_fileSystem->getDiskSpaceInfo(m_raTmpDir);
        const auto destSpaceinfo = m_fileSystem->getDiskSpaceInfo(info.targetPath);
        std::filesystem::space_info spaceInfoToCheck = tmpSpaceInfo.available > destSpaceinfo.available ? destSpaceinfo : tmpSpaceInfo;

        //Todo is there a better way to ensure enough space to decompress
        if ((!info.decompress && spaceInfoToCheck.available < info.sizeBytes) ||
            (info.decompress && spaceInfoToCheck.available < (info.sizeBytes * 2.5)))
        {
            std::stringstream spaceError;
            spaceError << "Not enough space to complete download action : sophos install disk has "  << tmpSpaceInfo.available <<
                " ,destination disk has " << destSpaceinfo.available;
            LOGWARN(spaceError.str());
            ActionsUtils::setErrorInfo(response, 1, spaceError.str(), "not_enough_space");
            return;
        }

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
            std::string error = "Downloaded file " + fileName + " cannot be accessed";
            ActionsUtils::setErrorInfo(response, 1, error, "access_denied");
            return "";
        }
        catch (const std::exception& exception)
        {
            std::stringstream error;
            error << "Unknown error when calculating digest of " << fileName << " :" << exception.what();
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return "";
        }

        assert(fileSha != "");

        if (fileSha != info.sha256)
        {
            std::stringstream shaError;
            shaError << "Sha256 provided in request (" << info.sha256 << ") doesnt match that of file downloaded (" << fileSha << ")";
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
                m_fileSystem->moveFile(extractedFile.front(), info.targetPath);
                auto extractedFileName = Common::FileSystem::basename(extractedFile.front());
                LOGINFO(info.targetPath << "/" << extractedFileName << " downloaded successfully");
            }
            else
            {
                std::stringstream error;
                error << "Error unzipping " << filePath << " due to ";

                if (ret == UNZ_BADPASSWORD)
                {
                    error << " bad password";
                    LOGWARN(error.str());
                    ActionsUtils::setErrorInfo(response, 1, error.str(), "invalid_password");
                }
                else if (ret == UNZ_BADZIPFILE)
                {
                    error << " bad archive";
                    LOGWARN(error.str());
                    ActionsUtils::setErrorInfo(response, 1, error.str(), "invalid_archive");
                }
                else
                {
                    error << " error no " << ret;
                    LOGWARN(error.str());
                    ActionsUtils::setErrorInfo(response, 3, error.str());
                }
            }
        }
        else
        {
            m_fileSystem->moveFile(filePath, info.targetPath);
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
            LOGWARN(httpresponse.error);
            ActionsUtils::setErrorInfo(response, 1, httpresponse.error);
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::FAILED)
        {
            LOGWARN(httpresponse.error);
            ActionsUtils::setErrorInfo(response, 3, httpresponse.error);
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
                error << "Failed to download " << url << " : Error code " << httpresponse.status;
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
}