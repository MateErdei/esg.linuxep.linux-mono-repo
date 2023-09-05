// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "SulDownloader/suldownloaderdata/ConfigurationData.h"
#include "SulDownloader/suldownloaderdata/ConnectionSelector.h"
#include "tests/Common/Helpers/ConfigurationDataBase.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

using namespace Common::Policy;

class ConnectionSelectorTest : public ConfigurationDataBase
{
public:
    void TearDown() override
    {
        Tests::restoreFileSystem();
    }


};

TEST_F(ConnectionSelectorTest, getConnectionCandidates)
{
    auto configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString("", ""));

    ConnectionSelector selector;
    auto [susCandidates, connectionCandidates] = selector.getConnectionCandidates(configurationData);

    // susCandidates should be ordered. With messagerelay first. NoProxy Should Not be included
    ASSERT_EQ(susCandidates.size(), 2);

    EXPECT_FALSE(susCandidates[0].isCacheUpdate());
    EXPECT_STREQ(susCandidates[0].getUpdateLocationURL().c_str(), "https://sus.sophosupd.com");
    EXPECT_EQ(susCandidates[0].getProxy().getUrl(), "https://cache.sophos.com/latest/warehouse");

    EXPECT_FALSE(susCandidates[1].isCacheUpdate());
    EXPECT_STREQ(susCandidates[1].getUpdateLocationURL().c_str(), "https://sus.sophosupd.com");
    EXPECT_EQ(susCandidates[1].getProxy().getUrl(), "noproxy:");

    ASSERT_EQ(connectionCandidates.size(), 2);

    EXPECT_TRUE(connectionCandidates[0].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[0].getUpdateLocationURL().c_str(), "https://cache.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[0].getProxy().getUrl(), "noproxy:"); // Update caches bypass proxy


    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(
        connectionCandidates[1].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[1].getProxy().getUrl(), "noproxy:");
}
TEST_F(ConnectionSelectorTest, getConnectionCandidatesDiscardInvalidUrlsempty)
{
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString("https://sus.sophosupd.com", ""));

    ConnectionSelector selector;
    auto [susCandidates, connectionCandidates] = selector.getConnectionCandidates(configurationData);

    ASSERT_EQ(susCandidates.size(), 0);
}

TEST_F(ConnectionSelectorTest, getConnectionCandidatesHandlesUrlWithNoslash)
{

    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString("https://sus.sophosupd.com", "sus.sophosupd.com"));

    ConnectionSelector selector;
    auto [susCandidates, connectionCandidates] = selector.getConnectionCandidates(configurationData);

    ASSERT_EQ(susCandidates.size(), 2);
}

TEST_F(ConnectionSelectorTest, getConnectionCandidatesDiscardInvalidUrlsTooLong)
{
    std::string longString(1025, 'a');
    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString("sus.sophosupd.com", longString));

    ConnectionSelector selector;
    auto [susCandidates, connectionCandidates] = selector.getConnectionCandidates(configurationData);

    ASSERT_EQ(susCandidates.size(), 0);
}

TEST_F(ConnectionSelectorTest, getConnectionCandidatesDiscardInvalidUrlsInvalidChar)
{
    auto configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(
        createJsonString("https://sus.sophosupd.com", "https://sus.@sophosupd.com"));

    ConnectionSelector selector;
    auto [susCandidates, connectionCandidates] = selector.getConnectionCandidates(configurationData);

    ASSERT_EQ(susCandidates.size(), 0);
}

TEST_F(ConnectionSelectorTest, getConnectionCandidatesWithOverride)
{
    std::vector<std::string> overrideContents = { "CDN_URL = https://localhost:8081",
                                                    "URLS = https://localhost:8081,https://localhost2:8081"};
    auto filesystemMock = std::make_unique<NiceMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("base/update/var/sdds3_override_settings.ini"))).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("savedproxy.config"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(HasSubstr("current_proxy"))).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, readLines(HasSubstr("base/update/var/sdds3_override_settings.ini"))).WillRepeatedly(Return(overrideContents));
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(filesystemMock));

    auto configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString("", ""));

    ConnectionSelector selector;
    auto [susCandidates, connectionCandidates] = selector.getConnectionCandidates(configurationData);

    // susCandidates should be ordered. With messagerelay first. NoProxy Should Not be included
    ASSERT_EQ(susCandidates.size(), 4);

    EXPECT_FALSE(susCandidates[0].isCacheUpdate());
    EXPECT_STREQ(susCandidates[0].getUpdateLocationURL().c_str(), "https://localhost:8081");
    EXPECT_EQ(susCandidates[0].getProxy().getUrl(), "https://cache.sophos.com/latest/warehouse");

    EXPECT_FALSE(susCandidates[1].isCacheUpdate());
    EXPECT_STREQ(susCandidates[1].getUpdateLocationURL().c_str(), "https://localhost2:8081");
    EXPECT_EQ(susCandidates[1].getProxy().getUrl(), "https://cache.sophos.com/latest/warehouse");

    EXPECT_FALSE(susCandidates[2].isCacheUpdate());
    EXPECT_STREQ(susCandidates[2].getUpdateLocationURL().c_str(), "https://localhost:8081");
    EXPECT_EQ(susCandidates[2].getProxy().getUrl(), "noproxy:");

    EXPECT_FALSE(susCandidates[3].isCacheUpdate());
    EXPECT_STREQ(susCandidates[3].getUpdateLocationURL().c_str(), "https://localhost2:8081");
    EXPECT_EQ(susCandidates[3].getProxy().getUrl(), "noproxy:");

    ASSERT_EQ(connectionCandidates.size(), 2);

    EXPECT_TRUE(connectionCandidates[0].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[0].getUpdateLocationURL().c_str(), "https://cache.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[0].getProxy().getUrl(), "noproxy:"); // Update caches bypass proxy

    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[1].getUpdateLocationURL().c_str(), "https://localhost:8081");
    EXPECT_EQ(connectionCandidates[1].getProxy().getUrl(), "noproxy:");
}

TEST_F(ConnectionSelectorTest, getConnectionCandidatesHandlesEnvProxy)
{
    auto configurationData = suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString("", ""));
    setenv("HTTPS_PROXY", "https://proxy.eng.sophos:8080", 1);
    ConnectionSelector selector;
    auto [susCandidates, connectionCandidates] = selector.getConnectionCandidates(configurationData);

    // susCandidates should be ordered. With messagerelay first. NoProxy Should Not be included
    ASSERT_EQ(susCandidates.size(), 3);

    EXPECT_FALSE(susCandidates[0].isCacheUpdate());
    EXPECT_STREQ(susCandidates[0].getUpdateLocationURL().c_str(), "https://sus.sophosupd.com");
    EXPECT_EQ(susCandidates[0].getProxy().getUrl(), "https://cache.sophos.com/latest/warehouse");

    EXPECT_FALSE(susCandidates[1].isCacheUpdate());
    EXPECT_STREQ(susCandidates[1].getUpdateLocationURL().c_str(), "https://sus.sophosupd.com");
    EXPECT_EQ(susCandidates[1].getProxy().getUrl(), "environment:");

    EXPECT_FALSE(susCandidates[2].isCacheUpdate());
    EXPECT_STREQ(susCandidates[2].getUpdateLocationURL().c_str(), "https://sus.sophosupd.com");
    EXPECT_EQ(susCandidates[2].getProxy().getUrl(), "noproxy:");


    ASSERT_EQ(connectionCandidates.size(), 3);

    EXPECT_TRUE(connectionCandidates[0].isCacheUpdate());
    EXPECT_STREQ(connectionCandidates[0].getUpdateLocationURL().c_str(), "https://cache.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[0].getProxy().getUrl(), "noproxy:"); // Update caches bypass proxy

    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(
        connectionCandidates[1].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[1].getProxy().getUrl(), "environment:");


    EXPECT_FALSE(connectionCandidates[2].isCacheUpdate());
    EXPECT_STREQ(
        connectionCandidates[2].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[2].getProxy().getUrl(), "noproxy:");
    unsetenv("HTTPS_PROXY");
}


TEST_F(ConnectionSelectorTest, getConnectionCandidatesHandlesSimpleProxyWithNonPassword)
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
                               "password": "password",
                               "proxyType": "1"
                                }
                               },)";

    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    ConnectionSelector selector;
    auto [susCandidates, connectionCandidates] = selector.getConnectionCandidates(configurationData);


    // susCandidates should be ordered. With messagerelay first. NoProxy Should Not be included
    ASSERT_EQ(susCandidates.size(), 3);

    EXPECT_FALSE(susCandidates[1].isCacheUpdate());
    EXPECT_STREQ(
        susCandidates[1].getUpdateLocationURL().c_str(), "https://sus.sophosupd.com");
    EXPECT_EQ(susCandidates[1].getProxy().getUrl(), "http://testproxy.com");
    EXPECT_STREQ(susCandidates[1].getProxy().getCredentials().getUsername().c_str(), "testproxyusername");
    EXPECT_STREQ(
        susCandidates[1].getProxy().getCredentials().getDeobfuscatedPassword().c_str(), "password");


    ASSERT_EQ(connectionCandidates.size(), 3);

    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(
        connectionCandidates[1].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[1].getProxy().getUrl(), "http://testproxy.com");
    EXPECT_STREQ(connectionCandidates[1].getProxy().getCredentials().getUsername().c_str(), "testproxyusername");
    EXPECT_STREQ(
        connectionCandidates[1].getProxy().getCredentials().getDeobfuscatedPassword().c_str(), "password");
}

TEST_F(ConnectionSelectorTest, getConnectionCandidatesHandlesSimpleProxyWithObfuscatedPassword)
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
                               "password": "CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk================",
                               "proxyType": "2"
                                }
                               },)";

    auto configurationData =
        suldownloaderdata::ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    ConnectionSelector selector;
    auto [susCandidates, connectionCandidates] = selector.getConnectionCandidates(configurationData);


    // susCandidates should be ordered. With messagerelay first. NoProxy Should Not be included
    ASSERT_EQ(susCandidates.size(), 3);

    EXPECT_FALSE(susCandidates[1].isCacheUpdate());
    EXPECT_STREQ(
        susCandidates[1].getUpdateLocationURL().c_str(), "https://sus.sophosupd.com");
    EXPECT_EQ(susCandidates[1].getProxy().getUrl(), "http://testproxy.com");
    EXPECT_STREQ(susCandidates[1].getProxy().getCredentials().getUsername().c_str(), "testproxyusername");
    EXPECT_STREQ(
        susCandidates[1].getProxy().getCredentials().getDeobfuscatedPassword().c_str(), "password");

    ASSERT_EQ(connectionCandidates.size(), 3);

    EXPECT_FALSE(connectionCandidates[1].isCacheUpdate());
    EXPECT_STREQ(
        connectionCandidates[1].getUpdateLocationURL().c_str(), "https://sophosupdate.sophos.com/latest/warehouse");
    EXPECT_EQ(connectionCandidates[1].getProxy().getUrl(), "http://testproxy.com");
    EXPECT_STREQ(connectionCandidates[1].getProxy().getCredentials().getUsername().c_str(), "testproxyusername");
    EXPECT_STREQ(
        connectionCandidates[1].getProxy().getCredentials().getDeobfuscatedPassword().c_str(), "password");
}
