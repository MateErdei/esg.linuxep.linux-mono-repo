// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ActionStructs.h"

#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"

#include <json.hpp>
#include <string>
namespace ResponseActionsImpl
{
    class UploadFileAction
    {
    public:
        UploadFileAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> requester);
        [[nodiscard]] std::string run(const std::string& actionJson);

    private:
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_client;
        void handleHttpResponse(const Common::HttpRequests::Response& httpresponse, nlohmann::json& response);
        void prepareAndUpload(const UploadInfo& info, nlohmann::json& response);
        std::string m_filename;
        std::string m_pathToUpload;
    };

}