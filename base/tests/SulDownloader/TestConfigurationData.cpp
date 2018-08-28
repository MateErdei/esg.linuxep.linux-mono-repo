/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <tests/Common/FileSystemImpl/MockFileSystem.h>

using namespace SulDownloader;

class ConfigurationDataTest : public ::testing::Test
{

public:
    std::string m_absInstallationPath;
    std::string m_primaryPath;
    std::string m_distPath;
    std::string m_absCertificatePath;
    std::string m_absSystemSslPath;
    std::string m_absCacheUpdatePath;

     void SetUp() override
    {

        m_absInstallationPath = "/installroot";;
        m_absCertificatePath = "/installroot/dev_certificates";
        m_absSystemSslPath = "/installroot/etc/ssl/certs";
        m_absCacheUpdatePath = "/installroot/etc/cachessl/certs";
        m_primaryPath = "/installroot/base/update/cache/primarywarehouse";
        m_distPath = "/installroot/base/update/cache/primary";

    }

    void TearDown() override
    {
        Common::FileSystem::restoreFileSystem();
    }

    MockFileSystem& setupFileSystemAndGetMock()
    {
        using ::testing::Ne;
        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, "/installroot"
        );

        auto filesystemMock = new NiceMock<MockFileSystem>();
        ON_CALL(*filesystemMock, isDirectory(m_absInstallationPath)).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isDirectory(m_primaryPath)).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isDirectory(m_distPath)).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isDirectory(m_absCertificatePath)).WillByDefault(Return(true));
        std::string empty;
        ON_CALL(*filesystemMock, exists(empty)).WillByDefault(Return(false));
        ON_CALL(*filesystemMock, exists(Ne(empty))).WillByDefault(Return(true));

        auto pointer = filesystemMock;
        Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
        return *pointer;
    }


    std::string createJsonString(const std::string & oldPartString, const std::string & newPartString)
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
                               "password": ""
                                }
                               },
                               "installationRootPath": "absInstallationPath",
                               "certificatePath": "absCertificatePath",
                               "systemSslPath": "absSystemSslPath",
                               "cacheUpdateSslPath": "absCacheUpdatePath",
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

        jsonString.replace(jsonString.find("absInstallationPath"), std::string("absInstallationPath").size(), m_absInstallationPath);
        jsonString.replace(jsonString.find("absCertificatePath"), std::string("absCertificatePath").size(), m_absCertificatePath);
        jsonString.replace(jsonString.find("absSystemSslPath"), std::string("absSystemSslPath").size(), m_absSystemSslPath);
        jsonString.replace(jsonString.find("absCacheUpdatePath"), std::string("absCacheUpdatePath").size(), m_absCacheUpdatePath);


        if(!oldPartString.empty())
        {
            size_t pos = jsonString.find(oldPartString);

            EXPECT_TRUE(pos != std::string::npos);

            jsonString.replace(pos, oldPartString.size(), newPartString);

        }

        return jsonString;
    }

    ::testing::AssertionResult configurationDataIsEquivalent( const char* m_expr,
                                                      const char* n_expr,
                                                      const SulDownloader::ConfigurationData & expected,
                                                      const SulDownloader::ConfigurationData & resulted)
    {
        std::stringstream s;
        s<< m_expr << " and " << n_expr << " failed: ";

        if ( expected.getLogLevel() != resulted.getLogLevel())
        {
            return ::testing::AssertionFailure() << s.str() << "log level differs";
        }

        if ( expected.getUpdateCacheSslCertificatePath() != resulted.getUpdateCacheSslCertificatePath())
        {
            return ::testing::AssertionFailure() << s.str() << "update cache certificate path differs";
        }

        if (expected.getCredentials() != resulted.getCredentials())
        {
            return ::testing::AssertionFailure() << s.str() << "credentials differ";
        }

        if (expected.getSophosUpdateUrls() != resulted.getSophosUpdateUrls())
        {
            return ::testing::AssertionFailure() << s.str() << "update urls differ";
        }

        if (expected.getLocalUpdateCacheUrls() != resulted.getLocalUpdateCacheUrls())
        {
            return ::testing::AssertionFailure() << s.str() << "update cache urls differ";
        }

        if (expected.getProxy().getUrl() != resulted.getProxy().getUrl())
        {
            return ::testing::AssertionFailure() << s.str() << "proxy urls differ";
        }

        if (expected.getProxy() != resulted.getProxy())
        {
            return ::testing::AssertionFailure() << s.str() << "proxy credentials differ";
        }

        if (expected.proxiesList() != resulted.proxiesList())
        {
            return ::testing::AssertionFailure() << s.str() << "proxy list differs";
        }

        if (expected.getInstallationRootPath() != resulted.getInstallationRootPath())
        {
            return ::testing::AssertionFailure() << s.str() << "installation root path differs";
        }

        if (expected.getLocalWarehouseRepository() != resulted.getLocalWarehouseRepository())
        {
            return ::testing::AssertionFailure() << s.str() << "local warehouse repository differs";
        }

        if (expected.getLocalDistributionRepository() != resulted.getLocalDistributionRepository())
        {
            return ::testing::AssertionFailure() << s.str() << "local distribution repository differs";
        }

        if (expected.getCertificatePath() != resulted.getCertificatePath())
        {
            return ::testing::AssertionFailure() << s.str() << "certificate path differs";
        }

        if (expected.getSystemSslCertificatePath() != resulted.getSystemSslCertificatePath())
        {
            return ::testing::AssertionFailure() << s.str() << "system ssl certificate path differs";
        }

        if (expected.getProductSelection() != resulted.getProductSelection())
        {
            return ::testing::AssertionFailure() << s.str() << "product selection differs";
        }

        if (expected.getInstallArguments() != resulted.getInstallArguments())
        {
            return ::testing::AssertionFailure() << s.str() << "install arguments differs";
        }

        return ::testing::AssertionSuccess();
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
    setupFileSystemAndGetMock();
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidAndCompleteJsonStringShouldReturnValidDataObjectThatContainsExpectedData )
{
    setupFileSystemAndGetMock();
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());

    EXPECT_STREQ(configurationData.getSophosUpdateUrls()[0].c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_STREQ(configurationData.getLocalUpdateCacheUrls()[0].c_str(), "https://cache.sophos.com/latest/warehouse");

    EXPECT_STREQ(configurationData.getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(configurationData.getCredentials().getPassword().c_str(), "password");

    EXPECT_STREQ(configurationData.getProxy().getUrl().c_str(), "noproxy:");
    EXPECT_STREQ(configurationData.getProxy().getCredentials().getUsername().c_str(), "");
    EXPECT_STREQ(configurationData.getProxy().getCredentials().getPassword().c_str(), "");

    EXPECT_STREQ(configurationData.getProductSelection()[0].Name.c_str(), "FD6C1066-E190-4F44-AD0E-F107F36D9D40");
    EXPECT_TRUE(configurationData.getProductSelection()[0].Primary);
    EXPECT_FALSE(configurationData.getProductSelection()[0].Prefix);
    EXPECT_STREQ(configurationData.getProductSelection()[0].releaseTag.c_str(), "RECOMMENDED");
    EXPECT_STREQ(configurationData.getProductSelection()[0].baseVersion.c_str(), "9");

    EXPECT_STREQ(configurationData.getProductSelection()[1].Name.c_str(), "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2");
    EXPECT_FALSE(configurationData.getProductSelection()[1].Primary);
    EXPECT_FALSE(configurationData.getProductSelection()[1].Prefix);
    EXPECT_STREQ(configurationData.getProductSelection()[1].releaseTag.c_str(), "RECOMMENDED");
    EXPECT_STREQ(configurationData.getProductSelection()[1].baseVersion.c_str(), "9");

    EXPECT_STREQ(configurationData.getProductSelection()[2].Name.c_str(), "1CD8A803");
    EXPECT_FALSE(configurationData.getProductSelection()[2].Primary);
    EXPECT_TRUE(configurationData.getProductSelection()[2].Prefix);
    EXPECT_STREQ(configurationData.getProductSelection()[2].releaseTag.c_str(), "RECOMMENDED");
    EXPECT_STREQ(configurationData.getProductSelection()[2].baseVersion.c_str(), "9");

    EXPECT_STREQ(configurationData.getSystemSslCertificatePath().c_str(), m_absSystemSslPath.c_str());
    EXPECT_STREQ(configurationData.getUpdateCacheSslCertificatePath().c_str(), m_absCacheUpdatePath.c_str());

    EXPECT_STREQ(configurationData.getInstallArguments()[0].c_str(), "--install-dir");
    EXPECT_STREQ(configurationData.getInstallArguments()[1].c_str(), "/opt/sophos-av");

}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidStringWithNoUpdateCacheShouldReturnValidDataObject )
{
    setupFileSystemAndGetMock();
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(R"("https://cache.sophos.com/latest/warehouse")", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheValueShouldFailValidation )
{
    setupFileSystemAndGetMock();
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(R"(https://cache.sophos.com/latest/warehouse)", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidStringWithNoSophosURLsShouldThrow )
{
    setupFileSystemAndGetMock();
    try
    {
        ConfigurationData::fromJsonSettings(createJsonString("""https://sophosupdate.sophos.com/latest/warehouse""", ""));
    }
    catch(SulDownloaderException &e)
    {
        EXPECT_STREQ("Sophos Location list cannot be empty", e.what());
    }
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptySophosUrlValueShouldFailValidation )
{
    setupFileSystemAndGetMock();
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("https://sophosupdate.sophos.com/latest/warehouse", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidEmptyCredentialsShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("credential": {
                               "username": "administrator",
                               "password": "password"
                               },)";

    std::string newString = R"("credential": {
                               "username": "",
                               "password": ""
                               },)";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidMissingCredentialDetailsShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("credential": {
                               "username": "administrator",
                               "password": "password"
                               },)";

    std::string newString = R"("credential": {
                               },)";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());

}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidMissingCredentialsShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("credential": {
                               "username": "administrator",
                               "password": "password"
                               },)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());


}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingProxyShouldReturnValidDataObject )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": ""
                                }
                               },)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyCertificatePathWillUseDefaultOne)
{
    setupFileSystemAndGetMock();
    std::string oldString = m_absCertificatePath;

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
    EXPECT_EQ(configurationData.getCertificatePath(),
              Common::ApplicationConfiguration::applicationPathManager().getUpdateCertificatesPath());


}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingCertificatePathWillUseDefaultOnde)
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("certificatePath": ")" + m_absCertificatePath + R"(",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
    EXPECT_EQ(configurationData.getCertificatePath(),
              Common::ApplicationConfiguration::applicationPathManager().getUpdateCertificatesPath());

}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptySystemSslCertPathShouldFailValidation )
{
    std::string oldString = m_absSystemSslPath;

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingSystemSslCertPathShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("systemSslPath": ")" + m_absSystemSslPath + R"(",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheSslCertPathShouldFailValidationWhenUsingUpdateCaches )
{
    setupFileSystemAndGetMock();
    std::string oldString = m_absCacheUpdatePath;

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingUpdateCacheSslCertPathShouldFailValidationWhenUsingUpdateCaches )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("cacheUpdateSslPath": ")" + m_absCacheUpdatePath + R"(",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheSslCertPathShouldNotFailValidationWhenNotUsingUpdateCaches )
{
    setupFileSystemAndGetMock();
    std::string oldString = m_absCacheUpdatePath;

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingUpdateCacheSslCertPathShouldNotFailValidationWhenNotUsingUpdateCaches )
{

    setupFileSystemAndGetMock();
    std::string oldString = R"("cacheUpdateSslPath": ")" + m_absCacheUpdatePath + R"(",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyReleaseTagShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = "RECOMMENDED";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());

}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingReleaseTagShouldNotFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("releaseTag": "RECOMMENDED",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    // default will set RECOMMENDED Value.
    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyBaseVersionShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = "9";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());


}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingBaseVersionShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("baseVersion": "9",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyPrimaryShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = "FD6C1066-E190-4F44-AD0E-F107F36D9D40";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingPrimaryShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",)";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyFullNamesShouldNotFailValidation )
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2")";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingFullNamesShouldNotFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("fullNames": [
                               "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
                               ],)";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyFullNamesValueShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyPrefixNamesShouldNotFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("1CD8A803")";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingPrefixNamesShouldNotFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("prefixNames": [
                               "1CD8A803"
                               ],)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyPrefixNameValueShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("1CD8A803")";
    std::string newString = R"("")";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyInstallArgumentsShouldFailValidation )
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("--install-dir",
                               "/opt/sophos-av")";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingInstallArgumentsShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ])";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyValueInInstallArgumentsShouldFailValidation )
{
    setupFileSystemAndGetMock();
    std::string oldString = "--install-dir";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithAddedUnknownDataShouldThrow )
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("baseVersion": "9",)";

    std::string newString = R"("baseVersion": "9",
            "UnknownDataItem": "UnknownString",)";

    EXPECT_THROW(ConfigurationData::fromJsonSettings(createJsonString(oldString, newString)), SulDownloaderException);

}

TEST_F(ConfigurationDataTest, serializeDeserialize )
{
    std::string originalString = createJsonString("","");
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(originalString);
    ConfigurationData afterSerializer = ConfigurationData::fromJsonSettings( ConfigurationData::toJsonSettings(configurationData));


    EXPECT_PRED_FORMAT2( configurationDataIsEquivalent, configurationData, afterSerializer);
}
