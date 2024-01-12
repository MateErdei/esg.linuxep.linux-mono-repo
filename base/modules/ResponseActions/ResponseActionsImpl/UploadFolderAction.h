// Copyright 2023-2024 Sophos Limited. All rights reserved.

#pragma once

#include "ActionStructs.h"

#include "Common/ProcessMonitoring/ISignalHandler.h"
#include "Common/SystemCallWrapper/ISystemCallWrapperFactory.h"
#include "Common/HttpRequests/IHttpRequester.h"

#include <nlohmann/json.hpp>
#include <string>
namespace ResponseActionsImpl
{
    class UploadFolderAction
    {
    public:
        UploadFolderAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> requester,
                           Common::ISignalHandlerSharedPtr sigHandler,
                           Common::SystemCallWrapper::ISystemCallWrapperSharedPtr systemCallWrapper);
        [[nodiscard]] nlohmann::json run(const std::string& actionJson);

    private:
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_client;
        void handleHttpResponse(const Common::HttpRequests::Response& httpresponse, nlohmann::json& response);
        Common::HttpRequests::Response doPutRequest(Common::HttpRequests::RequestConfig request);
        void prepareAndUpload(const UploadInfo& info, nlohmann::json& response);
        std::string m_filename;
        std::string m_pathToUpload;
        bool m_terminate = false;
        bool m_timeout = false;
        Common::ISignalHandlerSharedPtr m_SignalHandler;
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_SysCallWrapper;
    };

} // namespace ResponseActionsImpl