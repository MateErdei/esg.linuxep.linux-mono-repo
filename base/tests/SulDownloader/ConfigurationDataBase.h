/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <gtest/gtest.h>

class ConfigurationDataBase : public ::testing::Test
{
public:
    std::string m_absInstallationPath;
    std::string m_primaryPath;
    std::string m_distPath;
    std::string m_absCertificatePath;
    std::string m_absSystemSslPath;
    std::string m_absCacheUpdatePath;

    ConfigurationDataBase()
    {
        m_absInstallationPath = "/installroot";;
        m_absCertificatePath = "/installroot/dev_certificates";
        m_absSystemSslPath = "/installroot/etc/ssl/certs";
        m_absCacheUpdatePath = "/installroot/etc/cachessl/certs";
        m_primaryPath = "/installroot/base/update/cache/primarywarehouse";
        m_distPath = "/installroot/base/update/cache/primary";
    }

    std::string createJsonString(const std::string& oldPartString, const std::string& newPartString)
    {
        std::string jsonString = R"({
                               "sophosURLs": [
                               "https://sophosupdate.sophos.com/latest/warehouse"
                               ],
                               "updateCache": [
                               "https://cache.sophos.com/latest/warehouse"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },
                               "manifestNames" : ["manifest.dat", "telem-manifest.dat"],
                               "installationRootPath": "absInstallationPath",
                               "certificatePath": "absCertificatePath",
                               "systemSslPath": "absSystemSslPath",
                               "cacheUpdateSslPath": "absCacheUpdatePath",
                               "primarySubscription": {
                                "rigidName" : "BaseProduct-RigidName",
                                "baseVersion" : "9",
                                "tag" : "RECOMMENDED",
                                "fixVersion" : ""
                                },
                                "products": [
                                {
                                "rigidName" : "PrefixOfProduct-SimulateProductA",
                                "baseVersion" : "9",
                                "tag" : "RECOMMENDED",
                                "fixVersion" : ""
                                },
                                ],
                                "features": ["CORE", "MDR"],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

        jsonString.replace(
                jsonString.find("absInstallationPath"), std::string("absInstallationPath").size(), m_absInstallationPath
        );
        jsonString.replace(
                jsonString.find("absCertificatePath"), std::string("absCertificatePath").size(), m_absCertificatePath
        );
        jsonString.replace(
                jsonString.find("absSystemSslPath"), std::string("absSystemSslPath").size(), m_absSystemSslPath
        );
        jsonString.replace(
                jsonString.find("absCacheUpdatePath"), std::string("absCacheUpdatePath").size(), m_absCacheUpdatePath
        );

        if (!oldPartString.empty())
        {
            size_t pos = jsonString.find(oldPartString);

            EXPECT_TRUE(pos != std::string::npos);

            jsonString.replace(pos, oldPartString.size(), newPartString);
        }

        return jsonString;
    }
};