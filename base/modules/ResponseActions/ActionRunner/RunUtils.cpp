// Copyright 2023 Sophos Limited. All rights reserved.

#include "RunUtils.h"

#include "Logger.h"

#include "ResponseActions/ResponseActionsImpl/UploadFileAction.h"
#include "ResponseActions/ResponseActionsImpl/UploadFolderAction.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
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
    void RunUtils::sendResponse(const std::string& correlationId, const std::string& content)
    {
        LOGDEBUG("Command result: " << content);
        std::string tmpPath = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        std::string rootInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string targetDir = Common::FileSystem::join(rootInstall, "base/mcs/response");
        std::string fileName = "CORE_" + correlationId + "_response.json";
        std::string fullTargetName = Common::FileSystem::join(targetDir, fileName);

        Common::FileSystem::createAtomicFileToSophosUser(content, fullTargetName, tmpPath);
    }
} // namespace ActionRunner