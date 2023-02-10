// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>

#include <string>
namespace ResponseActionsImpl
{
    class UploadFileAction
    {
    public:
        UploadFileAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> requester);
        ~UploadFileAction() = default;
        [[nodiscard]] std::string run(const std::string& actionJson) const;
    private:
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_client;
    };

}