// Copyright 2023 Sophos Limited. All rights reserved.

#include "DownloadFileAction.h"

#include "ActionsUtils.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileTooLargeException.h>
#include <Common/ProxyUtils/ProxyUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/ZipUtilities/ZipUtils.h>

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

        auto* fs = Common::FileSystem::fileSystem();
        if (!fs->isFile(info.targetPath))
        {
            std::string error = info.targetPath + " is not a file";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response, 1, error, "invalid_path");
            return response.dump();
        }

        Common::UtilityImpl::FormattedTime time;
        u_int64_t start = time.currentEpochTimeInSecondsAsInteger();
        response["startedAt"] = start;

        prepareAndDownload(info, response);

        if (response["httpStatus"] == 0)
        {
            assert(!response.contains("errorType"));
            assert(!response.contains("errorMessage"));

            checkAndUnzip(info, response);
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

        std::string zipName = Common::FileSystem::basename(info.targetPath) + ".zip";
        std::string tmpdir = Common::ApplicationConfiguration::applicationPathManager().getResponseActionTmpPath();
        m_pathToDownload = Common::FileSystem::join(tmpdir, zipName);
        m_downloadFilename = Common::FileSystem::basename(m_pathToDownload);

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

        const auto spaceInfo = m_fileSystem->getDiskSpaceInfo(tmpdir);

        //Todo is there a better way to ensure enough space to decompress
        if ((!info.decompress && spaceInfo.available < info.sizeBytes) ||
            (info.decompress && spaceInfo.available < (info.sizeBytes * 2.5)))
        {
            std::stringstream spaceError;
            spaceError << "Not enough space on disk (" << spaceInfo.available << ") to complete download action";
            LOGWARN(spaceError.str());
            ActionsUtils::setErrorInfo(response, 1, spaceError.str(), "not_enough_space");
            return;
        }

        Common::HttpRequests::RequestConfig request{
            .url = info.url, .fileDownloadLocation = m_pathToDownload, .timeout = info.timeout
        };
        LOGINFO("Downloading file: " << m_pathToDownload << " to url: " << info.url);
        Common::HttpRequests::Response httpresponse;

        if (Common::ProxyUtils::updateHttpRequestWithProxyInfo(request))
        {
            std::stringstream message;
            message << "Downloading via proxy: " << request.proxy.value();
            LOGINFO(message.str());
            httpresponse = m_client->get(request);
            handleHttpResponse(httpresponse, response);

            if (response["result"] != 0)
            {
                LOGWARN("Connection with proxy failed going direct");
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

    void DownloadFileAction::checkAndUnzip(const DownloadInfo& info, nlohmann::json& response)
    {
        //Check sha256
        std::string fileSha = "";
        try
        {
            fileSha = m_fileSystem->calculateDigest(Common::SslImpl::Digest::sha256, m_pathToDownload);
        }
        catch (const Common::FileSystem::IFileSystemException&)
        {
            std::string error = "Downloaded file cannot be accessed";
            ActionsUtils::setErrorInfo(response, 1, error, "access_denied");
            return;
        }
        catch (const std::exception& exception)
        {
            std::stringstream error;
            error << "Unknown error when calculating digest of file :" << exception.what();
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return;
        }

        assert(fileSha != "");

        if (fileSha != info.sha256)
        {
            std::stringstream shaError;
            shaError << "Sha256 provided in request (" << info.sha256 << ") doesnt match that of file downloaded (" << fileSha << ")";
            ActionsUtils::setErrorInfo(response, 1, shaError.str(), "access_denied");
            return;
        }


        //decompress
        if (info.decompress)
        {
            int ret;
            try
            {
                if (info.password.empty())
                {
                    ret = Common::ZipUtilities::zipUtils().unzip(info.targetPath, m_pathToDownload);
                }
                else
                {
                    ret = Common::ZipUtilities::zipUtils().unzip(info.targetPath, m_pathToDownload, true, info.password);
                }
            }
            catch (const std::runtime_error&)
            {
                // logging is done in the zip util
                ret = 1;
            }

            //todo find out if error with password of invalid archive
            if (ret != 0)
            {
                std::stringstream error;
                error << "Error unzipping " << info.targetPath;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(response, 3, error.str());
            }
        }


    }

    void DownloadFileAction::handleHttpResponse(
        const Common::HttpRequests::Response& httpresponse,
        nlohmann::json& response)
    {

        if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT)
        {
            std::stringstream error;
            error << "Timeout downloading file: " << m_downloadFilename;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 2, error.str());
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::DOWNLOAD_TARGET_ALREADY_EXISTS)
        {
            LOGWARN(httpresponse.error);
            ActionsUtils::setErrorInfo(response, 1, httpresponse.error, "path_exists");
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
                LOGINFO("Download for " << m_pathToDownload << " succeeded");
            }
            else
            {
                std::stringstream error;
                error << "Failed to download file: " << m_downloadFilename << " with http error code " << httpresponse.status;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
            }
        }
        else
        {
            std::stringstream error;
            error << "Failed to download file: " << m_downloadFilename << " with error: " << httpresponse.error;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
        }
    }
}