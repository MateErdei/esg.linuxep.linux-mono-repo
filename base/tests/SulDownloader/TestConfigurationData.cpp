/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "gtest/gtest_pred_impl.h"

#include "SulDownloader/ConfigurationData.h"
#include "SulDownloader/SulDownloaderException.h"

using namespace SulDownloader;

class ConfigurationDataTest : public ::testing::Test
{
public:

    void SetUp() override
    {

    }
};

TEST_F( ConfigurationDataTest, fromJsonSettingsInvalidJsonStringThrows)
{
    try
    {
        ConfigurationData::fromJsonSettings("non json string");
    }
    catch(SulDownloaderException &e)
    {
        EXPECT_STREQ("Failed to process json message", e.what());
    }

}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidButEmptyJsonStringShouldThrow )
{
    try
    {
        ConfigurationData::fromJsonSettings("{}");
    }
    catch(SulDownloaderException &e)
    {
        EXPECT_STREQ("Sophos Location list cannot be empty", e.what());
    }
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidAndCompleteJsonStringShouldReturnValidDataObject )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidStringWithNoUpdateCacheShouldReturnValidDataObject )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheValueShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               ""
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidStringWithNoSophosURLsShouldThrow )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               ],
                               "updateCache": [
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";
    try
    {
        ConfigurationData::fromJsonSettings(jsonString);
    }
    catch(SulDownloaderException &e)
    {
        EXPECT_STREQ("Sophos Location list cannot be empty", e.what());
    }
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptySophosUrlValueShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               ""
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidEmptyCredentialsShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "",
                               "password": ""
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidMissingCredentialDetailsShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidMissingCredentialsShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingProxyShouldReturnValidDataObject )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyInstallPathShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingInstallPathShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyCertificatePathShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingCertificatePathShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptySystemSslCertPathShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingSystemSslCertPathShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheSslCertPathShouldFailValidationWhenUsingUpdateCaches )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingUpdateCacheSslCertPathShouldFailValidationWhenUsingUpdateCaches )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheSslCertPathShouldNotFailValidationWhenNotUsingUpdateCaches )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingUpdateCacheSslCertPathShouldNotFailValidationWhenNotUsingUpdateCaches )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyReleaseTagShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingReleaseTagShouldNotFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    // default will set RECOMMENDED Value.
    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyBaseVersionShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingBaseVersionShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyPrimaryShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingPrimaryShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyFullNamesShouldNotFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingFullNamesShouldNotFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyFullNamesValueShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "",
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyPrefixNamesShouldNotFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingPrefixNamesShouldNotFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyPrefixNameValueShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               ""
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyInstallArgumentsShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingInstallArgumentsShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyValueInInstallArgumentsShouldFailValidation )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "",
                               "/opt/sophos-av"
                               ]
                               })";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(jsonString);

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithAddedUnknownDataShouldThrow )
{
    std::string jsonString = R"({
                               "sophosURLs": [
                               "https://ostia.eng.sophos/latest/Virt-vShield"
                               ],
                               "updateCache": [
                               "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },
                               "installationRootPath": "/tmp/sophos",
                               "certificatePath": "/home/pair/dev_certificates",
                               "systemSslPath": "/etc/ssl/certs",
                               "cacheUpdateSslPath": "/etc/ssl/certs",
                               "releaseTag": "RECOMMENDED",
                               "baseVersion": "9",
                               "UnknownDataItem": "UnknownString",
                               "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
                               "fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],
                               "prefixNames": [
                               "1CD8A803"
                               ],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ]
                               })";

    EXPECT_THROW(ConfigurationData::fromJsonSettings(jsonString), SulDownloaderException);

}

