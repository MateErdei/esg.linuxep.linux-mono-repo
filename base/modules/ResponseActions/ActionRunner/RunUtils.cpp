// Copyright 2023 Sophos Limited. All rights reserved.

#include "RunUtils.h"

#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
#include "Common/ProcessMonitoringImpl/SignalHandler.h"
#include "Common/SystemCallWrapper/SystemCallWrapperFactory.h"
#include "ResponseActions/ResponseActionsImpl/DownloadFileAction.h"
#include "ResponseActions/ResponseActionsImpl/RunCommandAction.h"
#include "ResponseActions/ResponseActionsImpl/UploadFileAction.h"
#include "ResponseActions/ResponseActionsImpl/UploadFolderAction.h"

namespace ActionRunner
{
    nlohmann::json RunUtils::doUpload(const std::string& action)
    {
        auto curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        auto client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        ResponseActionsImpl::UploadFileAction uploadFileAction(client);
        return uploadFileAction.run(action);
    }

    nlohmann::json RunUtils::doUploadFolder(const std::string& action)
    {
        auto curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        auto client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        ResponseActionsImpl::UploadFolderAction uploadFolderAction(client);
        return uploadFolderAction.run(action);
    }

    nlohmann::json RunUtils::doDownloadFile(const std::string& action)
    {
        auto curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        auto client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        ResponseActionsImpl::DownloadFileAction downloadFileAction(client);
        return downloadFileAction.run(action);
    }

    nlohmann::json RunUtils::doRunCommand(const std::string& action, const std::string& correlationId)
    {
        auto sigHandler = std::make_shared<Common::ProcessMonitoringImpl::SignalHandler>();
        auto sysCallFactory = std::make_shared<Common::SystemCallWrapper::SystemCallWrapperFactory>();
        ResponseActionsImpl::RunCommandAction runCommandAction(sigHandler, sysCallFactory);
        return runCommandAction.run(action, correlationId);
    }

} // namespace ActionRunner