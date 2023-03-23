// Copyright 2023 Sophos Limited. All rights reserved.

#include "RunUtils.h"

#include "Logger.h"

#include "ResponseActions/ResponseActionsImpl/DownloadFileAction.h"
#include "ResponseActions/ResponseActionsImpl/RunCommandAction.h"
#include "ResponseActions/ResponseActionsImpl/UploadFileAction.h"
#include "ResponseActions/ResponseActionsImpl/UploadFolderAction.h"

#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>

namespace ActionRunner
{
    std::string RunUtils::doUpload(const std::string& action)
    {
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
            std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client =
            std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        ResponseActionsImpl::UploadFileAction uploadFileAction(client);
        return uploadFileAction.run(action);
    }

    std::string RunUtils::doUploadFolder(const std::string& action)
    {
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
            std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client =
            std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        ResponseActionsImpl::UploadFolderAction uploadFolderAction(client);
        return uploadFolderAction.run(action);
    }

    std::string RunUtils::doDownloadFile(const std::string& action)
    {
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
            std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client =
            std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        ResponseActionsImpl::DownloadFileAction downloadFileAction(client);
        return downloadFileAction.run(action);
    }

    std::string RunUtils::doRunCommand(const std::string& action, const std::string& correlationId)
    {
        return ResponseActionsImpl::RunCommandAction::run(action, correlationId);
    }
} // namespace ActionRunner