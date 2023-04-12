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
        bool initialChecks(const DownloadInfo& info, nlohmann::json& response);
        void download(const DownloadInfo& info, nlohmann::json& response);
        void handleHttpResponse(const Common::HttpRequests::Response& httpresponse, nlohmann::json& response);
        bool verifyFile(const DownloadInfo& info, nlohmann::json& response);
        void decompressAndMoveFile(const DownloadInfo& info, nlohmann::json& response);
        void makeDirAndMoveFile(nlohmann::json& response, Path destPath, const Path& filePathToMove);
        Path findBaseDir(const Path& path); //To be made more robust for general use and moved to Filesystem
        void removeTmpFiles();

        const Path m_raTmpDir = Common::ApplicationConfiguration::applicationPathManager().getResponseActionTmpPath();
        const Path m_tmpDownloadFile = m_raTmpDir + "/tmp_download.zip";
        const Path m_tmpExtractPath = m_raTmpDir + "/extract";
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_client;
        Common::FileSystem::IFileSystem* m_fileSystem = Common::FileSystem::fileSystem();
    };

}