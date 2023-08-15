// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once
#include <gtest/gtest.h>
#include "tests/Common/Helpers/LogInitializedTests.h"

class StateMachineDataBase : public LogOffInitializedTests
{
public:
    std::string m_primaryPath;
    std::string m_distPath;

    StateMachineDataBase()
    {
        m_primaryPath = "/installroot/base/update/cache/primarywarehouse";
        m_distPath = "/installroot/base/update/cache/primary";
    }

    std::string createJsonString(const std::string& oldPartString, const std::string& newPartString)
    {
        // Values set below represent values of a running system in which the last update was successful.

        std::string jsonString = R"({
 "DownloadFailedSinceTime": "0",
 "DownloadState": "0",
 "DownloadStateCredit": "72",
 "EventStateLastError": "0",
 "EventStateLastTime": "1610440731",
 "InstallFailedSinceTime": "0",
 "InstallState": "0",
 "InstallStateCredit": "3",
 "LastGoodInstallTime": "1610465945",
 "canSendEvent": true
})";

        return updateJsonString(jsonString, oldPartString, newPartString);

    }

    std::string createDefaultJsonString(const std::string& oldPartString, const std::string& newPartString)
    {
        // Values set below represent values of a running system in which the last update was successful.

        std::string jsonString = R"({
 "DownloadFailedSinceTime": "0",
 "DownloadState": "1",
 "DownloadStateCredit": "0",
 "EventStateLastError": "0",
 "EventStateLastTime": "0",
 "InstallFailedSinceTime": "0",
 "InstallState": "1",
 "InstallStateCredit": "0",
 "LastGoodInstallTime": "0",
 "canSendEvent": true
})";

        return updateJsonString(jsonString, oldPartString, newPartString);

    }


    std::string updateJsonString(std::string jsonString, const std::string& oldPartString, const std::string& newPartString)
    {
        if (!oldPartString.empty())
        {
            size_t pos = jsonString.find(oldPartString);

            EXPECT_TRUE(pos != std::string::npos);

            jsonString.replace(pos, oldPartString.size(), newPartString);
        }

        return jsonString;
    }
};