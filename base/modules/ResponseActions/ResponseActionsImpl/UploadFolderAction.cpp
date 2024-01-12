// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "UploadFolderAction.h"

#include "ActionStructs.h"
#include "ActionsUtils.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/Threads/NotifyPipe.h"
#include "Common/ProxyUtils/ProxyUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/ZipUtilities/ZipUtils.h"

#include <nlohmann/json.hpp>
#include <future>
namespace ResponseActionsImpl
{
    UploadFolderAction::UploadFolderAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> client,
                                           Common::ISignalHandlerSharedPtr sigHandler,
                                           Common::SystemCallWrapper::ISystemCallWrapperSharedPtr systemCallWrapper) :
            m_client(std::move(client)),
            m_SignalHandler(std::move(sigHandler)),
            m_SysCallWrapper(std::move(systemCallWrapper))
    {
    }

    nlohmann::json UploadFolderAction::run(const std::string& actionJson)
    {
        nlohmann::json response;
        response["type"] = ResponseActions::RACommon::UPLOAD_FOLDER_RESPONSE_TYPE;
        UploadInfo info;

        try
        {
            info = ActionsUtils::readUploadAction(actionJson, ActionType::UPLOADFOLDER);
        }
        catch (const InvalidCommandFormat& exception)
        {
            LOGWARN(exception.what());
            ActionsUtils::setErrorInfo(response, 1, "Error parsing command from Central");
            return response;
        }

        if (ActionsUtils::isExpired(info.expiration))
        {
            std::string error = "Upload folder action has expired";
            ;
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response, 4, error);
            return response;
        }

        auto* fs = Common::FileSystem::fileSystem();
        if (!fs->isDirectory(info.targetPath))
        {
            std::string error = info.targetPath + " is not a directory";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(response, 1, error, "invalid_path");
            return response;
        }

        Common::UtilityImpl::FormattedTime time;
        u_int64_t start = time.currentEpochTimeInSecondsAsInteger();
        response["startedAt"] = start;
        prepareAndUpload(info, response);
        if (fs->isFile(m_pathToUpload))
        {
            fs->removeFile(m_pathToUpload);
        }

        u_int64_t end = time.currentEpochTimeInSecondsAsInteger();
        response["duration"] = end - start;
        return response;
    }

    void UploadFolderAction::prepareAndUpload(const UploadInfo& info, nlohmann::json& response)
    {
        auto* fs = Common::FileSystem::fileSystem();
        std::string tmpdir = Common::ApplicationConfiguration::applicationPathManager().getResponseActionTmpPath();
        std::string zipName = Common::FileSystem::subdirNameFromPath(info.targetPath) + ".zip";
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
                ret = Common::ZipUtilities::zipUtils().zip(info.targetPath, m_pathToUpload, false, true, info.password);
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
            return;
        }

        response["sizeBytes"] = fs->fileSize(m_pathToUpload);
        if (response["sizeBytes"] > info.maxSize)
        {
            std::stringstream error;
            error << "Folder at path " << info.targetPath + " after being compressed is size " << response["sizeBytes"]
                  << " bytes which is above the size limit " << info.maxSize << " bytes";
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "exceed_size_limit");
            return;
        }

        std::string sha256;

        try
        {
            response["sha256"] = fs->calculateDigest(Common::SslImpl::Digest::sha256, m_pathToUpload);
        }
        catch (const Common::FileSystem::IFileSystemException&)
        {
            std::string error = "Zip file to be uploaded cannot be accessed";
            ActionsUtils::setErrorInfo(response, 1, error, "access_denied");
            return;
        }
        catch (const std::exception& exception)
        {
            std::stringstream error;
            error << "Unknown error when calculating digest of zip file :" << exception.what();
            ActionsUtils::setErrorInfo(response, 1, error.str());
            return;
        }

        Common::HttpRequests::Headers requestHeaders{ { "Content-Type", "application/zip" } };
        response["fileName"] = Common::FileSystem::basename(m_pathToUpload);

        Common::HttpRequests::RequestConfig request{
            .url = info.url, .headers = requestHeaders, .fileToUpload = m_pathToUpload, .timeout = info.timeout
        };
        LOGINFO(
            "Uploading folder: " << info.targetPath << " as zip file: " << m_pathToUpload << " to url: " << info.url);
        Common::HttpRequests::Response httpresponse;

        if (Common::ProxyUtils::updateHttpRequestWithProxyInfo(request))
        {
            LOGINFO("Uploading via proxy: " << request.proxy.value());
            httpresponse = doPutRequest(request);
            handleHttpResponse(httpresponse, response);
            if (m_terminate || m_timeout)
            {
                return;
            }
            if (response["result"] != 0)
            {
                LOGWARN("Connection with proxy failed, going direct");
                request.proxy = "";
                request.proxyPassword = "";
                request.proxyUsername = "";
                ActionsUtils::resetErrorInfo(response);
                httpresponse = doPutRequest(request);
                handleHttpResponse(httpresponse, response);
            }
        }
        else
        {
            LOGINFO("Uploading directly");
            httpresponse = doPutRequest(request);
            handleHttpResponse(httpresponse, response);
        }
        response["httpStatus"] = httpresponse.status;
    }

    Common::HttpRequests::Response UploadFolderAction::doPutRequest(Common::HttpRequests::RequestConfig request)
    {
        std::unique_ptr<Common::Threads::NotifyPipe> notifyPipe = std::make_unique<Common::Threads::NotifyPipe>();
        auto fut = std::async( std::launch::async, [this,request,&notifyPipe]()
                               {
                                   Common::HttpRequests::Response response = m_client->put(request);
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
                    LOGINFO("RunUploadFolderAction has received termination command");
                    m_terminate = true;
                    m_client->sendTerminate();
                    break;
                }
                if ((fds[1].revents & POLLIN) != 0)
                {
                    LOGINFO("RunUploadFolderAction has received termination command due to timeout");
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

    void UploadFolderAction::handleHttpResponse(
        const Common::HttpRequests::Response& httpresponse,
        nlohmann::json& response)
    {
        if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::TIMEOUT)
        {
            std::stringstream error;
            error << "Timeout uploading zip file: " << m_pathToUpload;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 2, error.str());
        }
        else if (httpresponse.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            if (httpresponse.status == 200)
            {
                response["result"] = 0;
                LOGINFO("Upload for " << m_pathToUpload << " succeeded");
            }
            else
            {
                std::stringstream error;
                error << "Failed to upload zip file: " << m_pathToUpload << " with http error code "
                      << httpresponse.status;
                LOGWARN(error.str());
                ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
            }
        }
        else
        {
            std::stringstream error;
            error << "Failed to upload zip file: " << m_pathToUpload << " with error: " << httpresponse.error;
            LOGWARN(error.str());
            ActionsUtils::setErrorInfo(response, 1, error.str(), "network_error");
        }
        if (m_timeout)
        {
            response["result"] = ResponseActions::RACommon::ResponseResult::TIMEOUT;
            return;
        }
    }
} // namespace ResponseActionsImpl