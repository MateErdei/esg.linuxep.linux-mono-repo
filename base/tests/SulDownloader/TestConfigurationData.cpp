/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "gtest/gtest_pred_impl.h"

#include "SulDownloader/ConfigurationData.h"
#include "SulDownloader/SulDownloaderException.h"
#include "../Common/TestHelpers/TempDir.h"


using namespace SulDownloader;

class ConfigurationDataTest : public ::testing::Test
{

public:
    std::string m_installRootRelPath;
    std::string m_certificateRelPath;
    std::string m_systemSslRelPath;
    std::string m_cacheUpdateSslRelPath;

    std::string m_absInstallationPath;
    std::string m_absCertificatePath;
    std::string m_absSystemSslPath;
    std::string m_absCacheUpdatePath;

    std::unique_ptr<Tests::TempDir> m_tempDir;

    ~ConfigurationDataTest()
    {
        m_tempDir.reset(nullptr);
    }

    void SetUp() override
    {
        m_installRootRelPath = "tmp/sophos-av";
        m_certificateRelPath ="tmp/dev_certificates";
        m_systemSslRelPath = "tmp/etc/ssl/certs";
        m_cacheUpdateSslRelPath = "tmp/etc/cachessl/certs";
        m_tempDir = Tests::TempDir::makeTempDir();

        m_tempDir->makeDirs(std::vector<std::string>{m_installRootRelPath,
                                                     m_certificateRelPath,
                                                     m_systemSslRelPath,
                                                     m_systemSslRelPath,
                                                     m_cacheUpdateSslRelPath,
                                                     "tmp/sophos-av/update/cache/PrimaryWarehouse",
                                                     "tmp/sophos-av/update/cache/Primary"});

        m_tempDir->createFile(m_certificateRelPath + "/ps_rootca.crt", "empty");
        m_tempDir->createFile(m_certificateRelPath + "/rootca.crt", "empty");

        m_absInstallationPath = m_tempDir->absPath(m_installRootRelPath);
        m_absCertificatePath = m_tempDir->absPath(m_certificateRelPath);
        m_absSystemSslPath = m_tempDir->absPath(m_systemSslRelPath);
        m_absCacheUpdatePath = m_tempDir->absPath(m_cacheUpdateSslRelPath);
    }

    void TearDown() override
    {

    }


    std::string createJsonString(const std::string & oldPartString, const std::string & newPartString)
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

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidAndCompleteJsonStringShouldReturnValidDataObjectThatContainsExpectedData )
{
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());

    EXPECT_STREQ(configurationData.getSophosUpdateUrls()[0].c_str(), "https://ostia.eng.sophos/latest/Virt-vShield");
    EXPECT_STREQ(configurationData.getLocalUpdateCacheUrls()[0].c_str(), "https://ostia.eng.sophos/latest/Virt-vShieldBroken");

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
    EXPECT_STREQ(configurationData.getInstallationRootPath().c_str(), m_absInstallationPath.c_str());
    EXPECT_STREQ(configurationData.getUpdateCacheSslCertificatePath().c_str(), m_absCacheUpdatePath.c_str());

    EXPECT_STREQ(configurationData.getInstallArguments()[0].c_str(), "--install-dir");
    EXPECT_STREQ(configurationData.getInstallArguments()[1].c_str(), "/opt/sophos-av");

}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidStringWithNoUpdateCacheShouldReturnValidDataObject )
{

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(R"("https://ostia.eng.sophos/latest/Virt-vShieldBroken")", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheValueShouldFailValidation )
{
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(R"(https://ostia.eng.sophos/latest/Virt-vShieldBroken)", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidStringWithNoSophosURLsShouldThrow )
{
    try
    {
        ConfigurationData::fromJsonSettings(createJsonString("""https://ostia.eng.sophos/latest/Virt-vShield""", ""));
    }
    catch(SulDownloaderException &e)
    {
        EXPECT_STREQ("Sophos Location list cannot be empty", e.what());
    }
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptySophosUrlValueShouldFailValidation )
{

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("https://ostia.eng.sophos/latest/Virt-vShield", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidEmptyCredentialsShouldFailValidation )
{
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

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyInstallPathShouldFailValidation )
{
    std::string oldString = m_absInstallationPath;

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingInstallPathShouldFailValidation )
{
    std::string oldString = R"("installationRootPath": ")" + m_absInstallationPath + R"(",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyCertificatePathShouldFailValidation )
{
    std::string oldString = m_absCertificatePath;

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());


}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingCertificatePathShouldFailValidation )
{
    std::string oldString = R"("certificatePath": ")" + m_absCertificatePath + R"(",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
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

    std::string oldString = R"("systemSslPath": ")" + m_absSystemSslPath + R"(",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheSslCertPathShouldFailValidationWhenUsingUpdateCaches )
{
    std::string oldString = m_absCacheUpdatePath;

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingUpdateCacheSslCertPathShouldFailValidationWhenUsingUpdateCaches )
{
    std::string oldString = R"("cacheUpdateSslPath": ")" + m_absCacheUpdatePath + R"(",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheSslCertPathShouldNotFailValidationWhenNotUsingUpdateCaches )
{
    std::string oldString = m_absCacheUpdatePath;

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingUpdateCacheSslCertPathShouldNotFailValidationWhenNotUsingUpdateCaches )
{


    std::string oldString = R"("cacheUpdateSslPath": ")" + m_absCacheUpdatePath + R"(",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyReleaseTagShouldFailValidation )
{

    std::string oldString = "RECOMMENDED";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());

}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingReleaseTagShouldNotFailValidation )
{
    std::string oldString = R"("releaseTag": "RECOMMENDED",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    // default will set RECOMMENDED Value.
    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyBaseVersionShouldFailValidation )
{

    std::string oldString = "9";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());


}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingBaseVersionShouldFailValidation )
{
    std::string oldString = R"("baseVersion": "9",)";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyPrimaryShouldFailValidation )
{
    std::string oldString = "FD6C1066-E190-4F44-AD0E-F107F36D9D40";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingPrimaryShouldFailValidation )
{
    std::string oldString = R"("primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",)";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyFullNamesShouldNotFailValidation )
{


    std::string oldString = R"("1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2")";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingFullNamesShouldNotFailValidation )
{
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
    std::string oldString = "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyPrefixNamesShouldNotFailValidation )
{
    std::string oldString = R"("1CD8A803")";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingPrefixNamesShouldNotFailValidation )
{

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
    std::string oldString = R"("1CD8A803")";
    std::string newString = R"("")";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyInstallArgumentsShouldFailValidation )
{

    std::string oldString = R"("--install-dir",
                               "/opt/sophos-av")";

    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingInstallArgumentsShouldFailValidation )
{
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
    std::string oldString = "--install-dir";
    std::string newString = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithAddedUnknownDataShouldThrow )
{
    std::string oldString = R"("baseVersion": "9",)";

    std::string newString = R"("baseVersion": "9",
            "UnknownDataItem": "UnknownString",)";

    EXPECT_THROW(ConfigurationData::fromJsonSettings(createJsonString(oldString, newString)), SulDownloaderException);

}

