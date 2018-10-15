/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include <SulDownloader/suldownloaderdata/ConnectionSelector.h>
#include "../Common/TestHelpers/TempDir.h"


using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class ConnectionSelectorTest : public ::testing::Test
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
                                                     "tmp/sophos-av/update/cache/primarywarehouse",
                                                     "tmp/sophos-av/update/cache/primary"}
        );

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


    ConfigurationData configFromJson(const std::string & oldPartString, const std::string & newPartString)
    {
        return ConfigurationData::fromJsonSettings(createJsonString(oldPartString, newPartString));
    }
};


TEST_F(ConnectionSelectorTest, getConnectionCandidatesShouldReturnValidCandidatesWithValidData) //NOLINT
{
    auto configurationData = configFromJson("", "");

    ConnectionSelector selector;
    auto connectionCandidates = selector.getConnectionCandidates(configurationData);

    // connectionCandidates should be ordered. With cache updates first.
    EXPECT_EQ(connectionCandidates.size(), 2);

    EXPECT_TRUE(connectionCandidates[0].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[0].getUpdateLocationURL().c_str(), "https://cache.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[0].getProxy().empty());

    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[1].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[1].getProxy().empty());


}


TEST_F(ConnectionSelectorTest, getConnectionCandidatesShouldReturnValidCandidatesWithValidDataNoProxyInfo) //NOLINT
{

    std::string oldString = R"("proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },)";

    std::string newString; // = "";

    auto configurationData = configFromJson(oldString, newString);

    ConnectionSelector selector;
    auto connectionCandidates = selector.getConnectionCandidates(configurationData);

    // connectionCandidates should be ordered. With cache updates first.
    EXPECT_EQ(connectionCandidates.size(), 2);

    EXPECT_TRUE(connectionCandidates[0].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[0].getUpdateLocationURL().c_str(), "https://cache.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[0].getProxy().empty());

    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[1].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[1].getProxy().empty());
}


TEST_F(ConnectionSelectorTest, getConnectionCandidatesShouldReturnValidCandidatesWithValidDataNoProxyInfoButEnvironmentVariable) //NOLINT
{

    std::string oldString = R"("proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },)";

    std::string newString; // = "";
    setenv( "HTTPS_PROXY", "https://proxy.eng.sophos:8080", 1);
    auto configurationData = configFromJson(oldString, newString);

    ConnectionSelector selector;
    auto connectionCandidates = selector.getConnectionCandidates(configurationData);

    // connectionCandidates should be ordered. With cache updates first.
    EXPECT_EQ(connectionCandidates.size(), 3);

    EXPECT_TRUE(connectionCandidates[0].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[0].getUpdateLocationURL().c_str(), "https://cache.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[0].getProxy().empty() );

    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[1].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[1].getProxy().getUrl(), "environment:" );

    EXPECT_FALSE(connectionCandidates[2].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[2].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[2].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[2].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[2].getProxy().empty() );

    unsetenv("HTTPS_PROXY");

}

TEST_F(ConnectionSelectorTest, getConnectionCandidatesShouldReturnValidCandidatesWithValidProxyDataWhenProxySetWithoutObfuscatedPassword) //NOLINT
{

    std::string oldString = R"("proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },)";

    std::string newString = R"("proxy": {
                               "url": "http://testproxy.com",
                               "credential": {
                               "username": "testproxyusername",
                               "password": "testproxypassword",
                               "proxyType": "1"
                                }
                               },)";

    suldownloaderdata::ConfigurationData configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    ConnectionSelector selector;
    auto connectionCandidates = selector.getConnectionCandidates(configurationData);

    // connectionCandidates should be ordered. With cache updates first.
    ASSERT_EQ(connectionCandidates.size(), 3);

    EXPECT_TRUE(connectionCandidates[0].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[0].getUpdateLocationURL().c_str(), "https://cache.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[0].getProxy().getUrl().empty() );
    EXPECT_STREQ(connectionCandidates[0].getProxy().getCredentials().getUsername().c_str(), "testproxyusername");
    EXPECT_STREQ(connectionCandidates[0].getProxy().getCredentials().getDeobfuscatedPassword().c_str(), "testproxypassword");


    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[1].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[1].getProxy().getUrl(), "http://testproxy.com" );
    EXPECT_STREQ(connectionCandidates[1].getProxy().getCredentials().getUsername().c_str(), "testproxyusername");
    EXPECT_STREQ(connectionCandidates[1].getProxy().getCredentials().getDeobfuscatedPassword().c_str(), "testproxypassword");

    EXPECT_FALSE(connectionCandidates[2].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[2].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[2].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[2].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[2].getProxy().empty() );

}


TEST_F(ConnectionSelectorTest, getConnectionCandidatesShouldReturnValidCandidatesWithValidProxyDataWhenProxySetWithObfuscatedPassword) //NOLINT
{

    std::string oldString = R"("proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },)";

    std::string newString = R"("proxy": {
                               "url": "http://testproxy.com",
                               "credential": {
                               "username": "testproxyusername",
                               "password": "CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=",
                               "proxyType": "2"
                                }
                               },)";

    suldownloaderdata::ConfigurationData configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    ConnectionSelector selector;
    auto connectionCandidates = selector.getConnectionCandidates(configurationData);

    // connectionCandidates should be ordered. With cache updates first.
    ASSERT_EQ(connectionCandidates.size(), 3);

    EXPECT_TRUE(connectionCandidates[0].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[0].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[0].getUpdateLocationURL().c_str(), "https://cache.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[0].getProxy().getUrl().empty() );
    EXPECT_STREQ(connectionCandidates[0].getProxy().getCredentials().getUsername().c_str(), "testproxyusername");
    EXPECT_STREQ(connectionCandidates[0].getProxy().getCredentials().getDeobfuscatedPassword().c_str(), "password");


    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[1].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[1].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[1].getProxy().getUrl(), "http://testproxy.com" );
    EXPECT_STREQ(connectionCandidates[1].getProxy().getCredentials().getUsername().c_str(), "testproxyusername");
    EXPECT_STREQ(connectionCandidates[1].getProxy().getCredentials().getDeobfuscatedPassword().c_str(), "password");

    EXPECT_FALSE(connectionCandidates[2].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[2].getCredentials().getUsername().c_str(), "administrator");
    EXPECT_STREQ(connectionCandidates[2].getCredentials().getPassword().c_str(), "password");
    EXPECT_STREQ(connectionCandidates[2].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_TRUE(connectionCandidates[2].getProxy().empty() );

}