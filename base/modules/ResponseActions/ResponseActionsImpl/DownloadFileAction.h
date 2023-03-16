// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ActionStructs.h"

#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>

#include <json.hpp>
#include <string>

namespace ResponseActionsImpl
{
    class DownloadFileAction
    {
    public:
        DownloadFileAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> requester);
        [[nodiscard]] std::string run(const std::string& actionJson);

    private:
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_client;
        void handleHttpResponse(const Common::HttpRequests::Response& httpresponse, nlohmann::json& response, const std::string& url);
        void prepareAndDownload(const DownloadInfo& info, nlohmann::json& response);
        Path verifyFile(const DownloadInfo& info, nlohmann::json& response);
        void decompressAndMoveFile(const Path& filePath, const DownloadInfo& info, nlohmann::json& response);
        void removeTmpFiles();

        const Path m_raTmpDir = Common::ApplicationConfiguration::applicationPathManager().getResponseActionTmpPath();

        Common::FileSystem::IFileSystem* m_fileSystem = Common::FileSystem::fileSystem();
    };

}