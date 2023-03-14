// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ActionStructs.h"

#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
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
        void handleHttpResponse(const Common::HttpRequests::Response& httpresponse, nlohmann::json& response);
        void prepareAndDownload(const DownloadInfo& info, nlohmann::json& response);
        void checkAndUnzip(const DownloadInfo& info, nlohmann::json& response);

        std::string m_downloadFilename = "";
        std::string m_pathToDownload = "";

        Common::FileSystem::IFileSystem* m_fileSystem = Common::FileSystem::fileSystem();
    };

}