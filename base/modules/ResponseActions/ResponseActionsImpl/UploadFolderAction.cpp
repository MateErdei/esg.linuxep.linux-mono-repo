// Copyright 2023 Sophos Limited. All rights reserved.

#include "UploadFolderAction.h"

#include "ActionStructs.h"
#include "ActionsUtils.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileTooLargeException.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/ZipUtilities/ZipUtils.h>

#include <json.hpp>
namespace ResponseActionsImpl
{
    UploadFolderAction::UploadFolderAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) :
        m_client(std::move(client))
    {
    }

    std::string UploadFolderAction::run(const std::string& actionJson) const
    {
        nlohmann::json response;
        response["type"] = "sophos.mgt.response.UploadFolder";
        UploadInfo info;

        try
        {
            info = ActionsUtils::readUploadAction(actionJson, UploadType::FOLDER);
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
            ActionsUtils::setErrorInfo(response, 4, error);
            return response.dump();
        }

        auto* fs = Common::FileSystem::fileSystem();
        if (!fs->isDirectory(info.targetPath))
        {
            std::string error = info.targetPath + " is not a directory";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response, 1, error, "invalid_path");
            return response.dump();
        }

        Common::UtilityImpl::FormattedTime time;
        u_int64_t start = time.currentEpochTimeInSecondsAsInteger();
        response["startedAt"] = start;
        std::string pathToUpload = "";

        std::string tmpdir = Common::ApplicationConfiguration::applicationPathManager().getResponseActionTmpPath();
        std::string zipName = Common::FileSystem::subdirNameFromPath(info.targetPath) + ".zip";
        pathToUpload = Common::FileSystem::join(tmpdir, zipName);
        int ret;
        try
        {
            if (info.password.empty())
            {
                ret = Common::ZipUtilities::zipUtils().zip(info.targetPath, pathToUpload, false);
            }
            else
            {
                ret = Common::ZipUtilities::zipUtils().zip(info.targetPath, pathToUpload, false, true, info.password);
            }
        }
        catch (const std::runtime_error&)
        {
            // logging is done in the zip util
            ret = 1;
        }
        if (ret != 0)
        {
            std::stringstream error;
            error << "Error zipping " << info.targetPath;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 3, error.str());
            if (fs->isFile(pathToUpload))
            {
                fs->removeFile(pathToUpload);
            }
            return response.dump();
        }

        response["sizeBytes"] = fs->fileSize(pathToUpload);
        if (response["sizeBytes"] > info.maxSize)
        {
            std::stringstream error;
            error << "Folder at path " << info.targetPath + " after being compressed is size " << response["sizeBytes"]
                  << " bytes which is above the size limit " << info.maxSize << " bytes";
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "exceed_size_limit");
            fs->removeFile(pathToUpload);
            return response.dump();
        }

        std::string sha256;

        try
        {
            response["sha256"] = fs->calculateDigest(Common::SslImpl::Digest::sha256, pathToUpload);
        }
        catch (const Common::FileSystem::IFileSystemException&)
        {
            std::string error = "Zip file to be uploaded cannot be accessed";
            ActionsUtils::setErrorInfo(response, 1, error, "access_denied");
            fs->removeFile(pathToUpload);
            return response.dump();
        }
        catch (const std::exception& exception)
        {
            std::stringstream error;
            error << "Unknown error when calculating digest of zip file :" << exception.what();
            ActionsUtils::setErrorInfo(response, 1, error.str());
            fs->removeFile(pathToUpload);
            return response.dump();
        }

        Common::HttpRequests::RequestConfig request{ .url = info.url,
                                                     .fileToUpload = pathToUpload,
                                                     .timeout = info.timeout };
        LOGINFO("Uploading folder: " << info.targetPath << " as zip file: " << pathToUpload << " to url: " << info.url);
        Common::HttpRequests::Response httpresponse = m_client->put(request);
        std::string filename = Common::FileSystem::basename(pathToUpload);
        response["fileName"] = filename;

        if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT)
        {
            std::stringstream error;
            error << "Timeout Uploading zip file: " << filename;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 2, error.str());
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            if (httpresponse.status == 200)
            {
                response["result"] = 0;
                LOGINFO("Upload for " << pathToUpload << " succeeded");
            }
            else
            {
                std::stringstream error;
                error << "Failed to upload zip file: " << filename << " with http error code " << httpresponse.status;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
            }
        }
        else
        {
            std::stringstream error;
            error << "Failed to upload zip file: " << filename << " with error code " << httpresponse.errorCode;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
        }

        fs->removeFile(pathToUpload);

        response["httpStatus"] = httpresponse.status;
        u_int64_t end = time.currentEpochTimeInSecondsAsInteger();
        response["duration"] = end - start;
        return response.dump();
    }

} // namespace ResponseActionsImpl