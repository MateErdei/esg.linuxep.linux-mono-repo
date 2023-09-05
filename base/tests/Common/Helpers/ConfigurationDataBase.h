// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once
#include "tests/Common/Helpers/LogInitializedTests.h"

#include <gtest/gtest.h>

class ConfigurationDataBase : public LogOffInitializedTests
{
public:
    std::string m_primaryPath;
    std::string m_distPath;

    ConfigurationDataBase()
    {
        m_primaryPath = "/installroot/base/update/cache/primarywarehouse";
        m_distPath = "/installroot/base/update/cache/primary";
    }

    std::string createJsonString(const std::string& oldPartString, const std::string& newPartString)
    {
        std::string jsonString = R"({
                               "sophosCdnURLs": [
                               "https://sophosupdate.sophos.com/latest/warehouse"
                               ],
                                "sophosSusURL": "https://sus.sophosupd.com",
                               "updateCache": [
                               "https://cache.sophos.com/latest/warehouse"
                               ],
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },
                               "manifestNames" : ["manifest.dat"],
                               "optionalManifestNames" : ["flags_manifest.dat"],
                               "primarySubscription": {
                                "rigidName" : "BaseProduct-RigidName",
                                "baseVersion" : "9",
                                "tag" : "RECOMMENDED",
                                "fixedVersion" : ""
                                },
                                "products": [
                                {
                                "rigidName" : "PrefixOfProduct-SimulateProductA",
                                "baseVersion" : "9",
                                "tag" : "RECOMMENDED",
                                "fixedVersion" : ""
                                },
                                ],
                                "features": ["CORE", "MDR"],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

        if (!oldPartString.empty())
        {
            size_t pos = jsonString.find(oldPartString);

            EXPECT_TRUE(pos != std::string::npos);

            jsonString.replace(pos, oldPartString.size(), newPartString);
        }

        return jsonString;
    }
};