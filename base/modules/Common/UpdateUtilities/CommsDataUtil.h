// Copyright 2023 Sophos All rights reserved.

#pragma once

#include <string>

namespace Common::UpdateUtilities
{
    class CommsDataUtil
    {
    public:
        static void writeCommsTelemetryIniFile(
                const std::string &fileName,
                bool usedProxy,
                bool usedUpdateCache,
                bool usedMessageRelay,
                const std::string &proxyOrMessageRelayURL = "");
    };
}