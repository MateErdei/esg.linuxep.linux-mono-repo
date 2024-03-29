// Copyright 2023-2024 Sophos Limited. All rights reserved.
#pragma once

#include "Common/FileSystem/IFileSystem.h"
#include "Common/HttpRequests/IHttpRequester.h"
#include "Common/OSUtilities/IPlatformUtils.h"


#include <string>
#include <vector>

namespace thininstaller::telemetry
{
    class Telemetry
    {
    public:
        Telemetry(std::vector<std::string> args, Common::HttpRequests::IHttpRequesterPtr httpRequester);
        int run(Common::FileSystem::IFileSystem* fs, const Common::OSUtilities::IPlatformUtils& platform);
        std::string json();
        std::string url();

    private:
        std::string json_;
        std::string url_;
        std::vector<std::string> args_;
        Common::HttpRequests::IHttpRequesterPtr requester_;
        void sendTelemetry(const std::string& url, const std::string& json, const std::string& proxy="");
    };
}
