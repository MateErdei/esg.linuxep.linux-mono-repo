// Copyright 2023 Sophos Limited. All rights reserved.

#include "UploadFileAction.h"
#include "ActionStructs.h"
#include "ActionsUtils.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileTooLargeException.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>

#include <json.hpp>
namespace ResponseActionsImpl
{
    std::string UploadFileAction::run(const std::string& actionJson) const
    {
        nlohmann::json response;
        response["type"] = "sophos.mgt.response.UploadFile";
        ActionsUtils actionsUtils;
        UploadInfo info;

        try
        {
            info = actionsUtils.readUploadAction(actionJson, UploadType::FILE);
        }
        catch (const std::runtime_error& exception)
        {
            LOGWARN(exception.what());
            actionsUtils.setErrorInfo(response,1,"invalid_path","Error parsing command from Central");
            return response.dump();
        }

        if (actionsUtils.isExpired(info.expiration))
        {
            std::string error = "Action has expired";
            LOGWARN(error);
            actionsUtils.setErrorInfo(response,4,error);
            return response.dump();
        }

        auto fs = Common::FileSystem::fileSystem();
        if (!fs->isFile(info.targetPath))
        {
            std::string error = info.targetPath + " is not a file";
            LOGWARN(error);
            actionsUtils.setErrorInfo(response,1,error,"invalid_path");
            return response.dump();
        }

        response["sizeBytes"] = fs->fileSize(info.targetPath);
        if (response["sizeBytes"] > info.maxSize)
        {
            std::stringstream error;
            error << info.targetPath + " is above the size limit" << info.maxSize;
            LOGWARN(error.str());
            actionsUtils.setErrorInfo(response,1,error.str(),"exceed_size_limit");
            return response.dump();
        }

        Common::UtilityImpl::FormattedTime time;
        u_int64_t start = time.currentEpochTimeInSecondsAsInteger();
        response["startedAt"] = start;
        std::string sha256;
        //scoping so we don't keep content in memory any longer than we have to
        {
            try
            {
                std::string content = fs->readFile(info.targetPath, 1024 * 1024 * 1024);
                sha256 = fs->calculateDigest(Common::SslImpl::Digest::sha256, content);
            }
            catch (const Common::FileSystem::IFileTooLargeException&)
            {
                std::string error = "File to be uploaded is being written to and has gone over the max size";
                actionsUtils.setErrorInfo(response,1,error,"exceed_size_limit");
            }
        }
        response["sha256"] = sha256;

        Common::HttpRequests::RequestConfig request{ .url = info.url, .fileToUpload=info.targetPath, .timeout = info.timeout};
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
            std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client =
            std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        Common::HttpRequests::Response httpresponse = client->put(request);
        std::string filename = Common::FileSystem::basename(info.targetPath);
        response["fileName"] = filename;
        if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            response["result"] = 0;
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT)
        {
            std::stringstream error;
            error << "Timeout Uploading file: " << filename;
            actionsUtils.setErrorInfo(response,2,error.str());
        }
        else
        {
            std::stringstream error;
            error << "Failed to upload file: " << filename;
            actionsUtils.setErrorInfo(response,1,error.str(),"network_error");
        }

        response["httpStatus"] = httpresponse.status;
        u_int64_t end = time.currentEpochTimeInSecondsAsInteger();
        response["duration"] = start - end;
        return response.dump();
    }

}