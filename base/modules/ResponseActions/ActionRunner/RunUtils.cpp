// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "RunUtils.h"

#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
#include "Common/ProcessMonitoringImpl/SignalHandler.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"
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
        auto sigHandler = std::make_shared<Common::ProcessMonitoringImpl::SignalHandler>();
        auto sysCallFactory = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
        ResponseActionsImpl::UploadFileAction uploadFileAction(client, sigHandler, sysCallFactory);
        return uploadFileAction.run(action);
    }

    nlohmann::json RunUtils::doUploadFolder(const std::string& action)
    {
        auto curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        auto client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        auto sigHandler = std::make_shared<Common::ProcessMonitoringImpl::SignalHandler>();
        auto sysCallFactory = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
        ResponseActionsImpl::UploadFolderAction uploadFolderAction(client, sigHandler, sysCallFactory);
        return uploadFolderAction.run(action);
    }

    nlohmann::json RunUtils::doDownloadFile(const std::string& action)
    {
        auto curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        auto client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        auto sigHandler = std::make_shared<Common::ProcessMonitoringImpl::SignalHandler>();
        auto sysCallFactory = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
        ResponseActionsImpl::DownloadFileAction downloadFileAction(client, sigHandler, sysCallFactory);
        return downloadFileAction.run(action);
    }

    nlohmann::json RunUtils::doRunCommand(const std::string& action, const std::string& correlationId)
    {
        auto sigHandler = std::make_shared<Common::ProcessMonitoringImpl::SignalHandler>();
        auto sysCallFactory = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
        ResponseActionsImpl::RunCommandAction runCommandAction(sigHandler, sysCallFactory);
        return runCommandAction.run(action, correlationId);
    }

} // namespace ActionRunner