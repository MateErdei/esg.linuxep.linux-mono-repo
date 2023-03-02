// Copyright 2023 Sophos Limited. All rights reserved.

#include "UploadFileAction.h"

#include "ActionStructs.h"
#include "ActionsUtils.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileTooLargeException.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/ZipUtilities/ZipUtils.h>

#include <json.hpp>
namespace ResponseActionsImpl
{
    UploadFileAction::UploadFileAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> client):
        m_client(std::move(client)){}

    std::string UploadFileAction::run(const std::string& actionJson) const
    {
        nlohmann::json response;
        response["type"] = "sophos.mgt.response.UploadFile";
        UploadInfo info;

        try
        {
            info = ActionsUtils::readUploadAction(actionJson, UploadType::FILE);
        }
        catch (const InvalidCommandFormat& exception)
        {
            LOGWARN(exception.what());
            ActionsUtils::setErrorInfo(response, 1, "Error parsing command from Central");
            return response.dump();
        }

        if (ActionsUtils::isExpired(info.expiration))
        {
            std::string error = "Action has expired";
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
        std::string pathToUpload = "";
        std::string contentType;
        if (info.compress)
        {
            contentType = "application/zip";
            std::string tmpdir = Common::ApplicationConfiguration::applicationPathManager().getResponseActionTmpPath();
            std::string zipName = Common::FileSystem::basename(info.targetPath) + ".zip";
            pathToUpload = Common::FileSystem::join(tmpdir, zipName);
            int ret;
            if (info.password.empty())
            {
                ret = Common::ZipUtilities::ZipUtils::zip(info.targetPath, pathToUpload);
            }
            else
            {
                ret = Common::ZipUtilities::ZipUtils::zip(info.targetPath, pathToUpload, true, info.password);
            }
            if (ret != 0)
            {
                std::stringstream error;
                error << "Error zipping " << info.targetPath;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(response, 3, error.str());
                return response.dump();
            }
        }
        else
        {
            contentType = "application/octet-stream";
            pathToUpload = info.targetPath;
        }
        response["sizeBytes"] = fs->fileSize(pathToUpload);
        if (response["sizeBytes"] > info.maxSize)
        {
            std::stringstream error;
            error << "File at path " << pathToUpload + " is size " << response["sizeBytes"]
                  << " bytes which is above the size limit " << info.maxSize << " bytes";
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "exceed_size_limit");
            return response.dump();
        }

        std::string sha256;

        try
        {
            response["sha256"] = fs->calculateDigest(Common::SslImpl::Digest::sha256, pathToUpload);
        }
        catch (const Common::FileSystem::IFileSystemException&)
        {
            std::string error = "File to be uploaded cannot be accessed";
            ActionsUtils::setErrorInfo(response, 1, error, "access_denied");
            return response.dump();
        }
        catch (const std::exception& exception)
        {
            std::stringstream error;
            error << "Unknown error when calculating digest of file :" << exception.what();
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return response.dump();
        }

        Common::HttpRequests::Headers requestHeaders{ { "Content-Type", contentType } };
        Common::HttpRequests::RequestConfig request{
            .url = info.url, .headers = requestHeaders, .fileToUpload = pathToUpload, .timeout = info.timeout
        };
        LOGINFO("Uploading file: " << pathToUpload << " to url: " << info.url);
        Common::HttpRequests::Response httpresponse = m_client->put(request);
        std::string filename = Common::FileSystem::basename(pathToUpload);
        response["fileName"] = filename;

        if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT)
        {
            std::stringstream error;
            error << "Timeout Uploading file: " << filename;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 2, error.str());
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            if (httpresponse.status == Common::HttpRequests::HTTP_STATUS_OK)
            {
                response["result"] = 0;
                LOGINFO("Upload for " << pathToUpload << " succeeded");
            }
            else
            {
                std::stringstream error;
                error << "Failed to upload file: " << filename << " with http error code " << httpresponse.status;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
            }
        }
        else
        {
            std::stringstream error;
            error << "Failed to upload file: " << filename << " with error: " << httpresponse.error;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
        }

        if (info.compress)
        {
            fs->removeFile(pathToUpload);
        }
        response["httpStatus"] = httpresponse.status;
        u_int64_t end = time.currentEpochTimeInSecondsAsInteger();
        response["duration"] = end - start;
        return response.dump();
    }

}