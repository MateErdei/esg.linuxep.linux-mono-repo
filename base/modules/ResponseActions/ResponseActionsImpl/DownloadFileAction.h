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
        [[nodiscard]] nlohmann::json run(const std::string& actionJson);

    private:
        bool initialChecks(const DownloadInfo& info, nlohmann::json& response);
        void download(const DownloadInfo& info, nlohmann::json& response);
        void handleHttpResponse(const Common::HttpRequests::Response& httpresponse, nlohmann::json& response);
        bool verifyFile(const DownloadInfo& info, nlohmann::json& response);
        void decompressAndMoveFile(const DownloadInfo& info, nlohmann::json& response);


        bool makeDestDirectory(nlohmann::json& response, const Path& destDir);
        void moveFile(nlohmann::json& response, const Path& destDir, const Path& fileName, const Path& filePathToMove);

        Path findBaseDir(const Path& path); //To be made more robust for general use and moved to Filesystem
        void removeTmpFiles();


        const Path m_raTmpDir = Common::ApplicationConfiguration::applicationPathManager().getResponseActionTmpPath();
        const Path m_tmpDownloadFile = m_raTmpDir + "/tmp_download.zip";
        const Path m_tmpExtractPath = m_raTmpDir + "/extract";
        const Path m_archiveFileName = "download.zip";

        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_client;
        Common::FileSystem::IFileSystem* m_fileSystem = Common::FileSystem::fileSystem();
    };

}