// Copyright 2023 Sophos Limited. All rights reserved.

#include "UploadFileAction.h"

#include "ActionsUtils.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/ProxyUtils/ProxyUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/ZipUtilities/ZipUtils.h"

namespace ResponseActionsImpl
{
    UploadFileAction::UploadFileAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) :
        m_client(std::move(client))
    {
    }

    nlohmann::json UploadFileAction::run(const std::string& actionJson)
    {
        nlohmann::json response;
        response["type"] = ResponseActions::RACommon::UPLOAD_FILE_RESPONSE_TYPE;
        UploadInfo info;

        try
        {
            info = ActionsUtils::readUploadAction(actionJson, ActionType::UPLOADFILE);
        }
        catch (const InvalidCommandFormat& exception)
        {
            LOGWARN(exception.what());
            ActionsUtils::setErrorInfo(
                response,
                static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR),
                "Error parsing command from Central");
            return response;
        }

        if (ActionsUtils::isExpired(info.expiration))
        {
            std::string error = "Upload file action has expired";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(
                response, static_cast<int>(ResponseActions::RACommon::ResponseResult::EXPIRED), error);
            return response;
        }

        auto* fs = Common::FileSystem::fileSystem();
        if (!fs->isFile(info.targetPath))
        {
            std::string error = info.targetPath + " is not a file";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(
                response, static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR), error, "invalid_path");
            return response;
        }

        Common::UtilityImpl::FormattedTime time;
        u_int64_t start = time.currentEpochTimeInSecondsAsInteger();
        response["startedAt"] = start;

        prepareAndUpload(info, response);

        if (info.compress)
        {
            if (fs->isFile(m_pathToUpload))
            {
                fs->removeFile(m_pathToUpload);
            }
        }

        u_int64_t end = time.currentEpochTimeInSecondsAsInteger();
        response["duration"] = end - start;
        return response;
    }

    void UploadFileAction::prepareAndUpload(const UploadInfo& info, nlohmann::json& response)
    {
        auto* fs = Common::FileSystem::fileSystem();
        std::string contentType;
        if (info.compress)
        {
            contentType = "application/zip";
            std::string tmpdir = Common::ApplicationConfiguration::applicationPathManager().getResponseActionTmpPath();
            std::string zipName = Common::FileSystem::basename(info.targetPath) + ".zip";
            m_pathToUpload = Common::FileSystem::join(tmpdir, zipName);
            int ret;
            try
            {
                if (info.password.empty())
                {
                    ret = Common::ZipUtilities::zipUtils().zip(info.targetPath, m_pathToUpload, false);
                }
                else
                {
                    ret = Common::ZipUtilities::zipUtils().zip(
                        info.targetPath, m_pathToUpload, false, true, info.password);
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
                ActionsUtils::setErrorInfo(
                    response, static_cast<int>(ResponseActions::RACommon::ResponseResult::INTERNAL_ERROR), error.str());
                return;
            }
        }
        else
        {
            contentType = "application/octet-stream";
            m_pathToUpload = info.targetPath;
        }
        response["sizeBytes"] = fs->fileSize(m_pathToUpload);
        if (response["sizeBytes"] > info.maxSize)
        {
            std::stringstream error;
            error << "File at path " << m_pathToUpload + " is size " << response["sizeBytes"]
                  << " bytes which is above the size limit " << info.maxSize << " bytes";
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(
                response,
                static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR),
                error.str(),
                "exceed_size_limit");
            return;
        }

        std::string sha256;

        try
        {
            response["sha256"] = fs->calculateDigest(Common::SslImpl::Digest::sha256, m_pathToUpload);
        }
        catch (const Common::FileSystem::IFileSystemException&)
        {
            std::string error = "File to be uploaded cannot be accessed";
            ActionsUtils::setErrorInfo(
                response, static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR), error, "access_denied");
            return;
        }
        catch (const std::exception& exception)
        {
            std::stringstream error;
            error << "Unknown error when calculating digest of file :" << exception.what();
            ActionsUtils::setErrorInfo(
                response, static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR), error.str());
            return;
        }

        m_filename = Common::FileSystem::basename(m_pathToUpload);
        response["fileName"] = m_filename;

        Common::HttpRequests::Headers requestHeaders{ { "Content-Type", contentType } };
        Common::HttpRequests::RequestConfig request{
            .url = info.url, .headers = requestHeaders, .fileToUpload = m_pathToUpload, .timeout = info.timeout
        };
        auto logLevel = getResponseActionsImplLogger().getChainedLogLevel();
        if(logLevel <= Common::Logging::DEBUG)
        {
            LOGDEBUG("Uploading file: " << m_pathToUpload << " to url: " << info.url);
        }
        else
        {
            LOGINFO("Uploading file: " << m_pathToUpload);
        }
        Common::HttpRequests::Response httpresponse;

        if (Common::ProxyUtils::updateHttpRequestWithProxyInfo(request))
        {
            LOGINFO("Uploading via proxy: " << request.proxy.value());
            httpresponse = m_client->put(request);
            handleHttpResponse(httpresponse, response);
            if (response["result"] != 0)
            {
                LOGWARN("Connection with proxy failed, going direct");
                request.proxy = "";
                request.proxyPassword = "";
                request.proxyUsername = "";
                ActionsUtils::resetErrorInfo(response);
                httpresponse = m_client->put(request);
                handleHttpResponse(httpresponse, response);
            }
        }
        else
        {
            LOGINFO("Uploading directly");
            httpresponse = m_client->put(request);
            handleHttpResponse(httpresponse, response);
        }
        response["httpStatus"] = httpresponse.status;
    }

    void UploadFileAction::handleHttpResponse(
        const Common::HttpRequests::Response& httpResponse,
        nlohmann::json& response)
    {
        if (httpResponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT)
        {
            std::stringstream error;
            error << "Timeout Uploading file: " << m_filename;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(
                response, static_cast<int>(ResponseActions::RACommon::ResponseResult::TIMEOUT), error.str());
        }
        else if (httpResponse.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            if (httpResponse.status == Common::HttpRequests::HTTP_STATUS_OK)
            {
                response["result"] = 0;
                LOGINFO("Upload for " << m_pathToUpload << " succeeded");
            }
            else
            {
                std::stringstream error;
                error << "Failed to upload file: " << m_filename << " with http error code " << httpResponse.status;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(
                    response,
                    static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR),
                    error.str(),
                    "network_error");
            }
        }
        else
        {
            std::stringstream error;
            error << "Failed to upload file: " << m_filename << " with error: " << httpResponse.error;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(
                response,
                static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR),
                error.str(),
                "network_error");
        }
    }
} // namespace ResponseActionsImpl