// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/HttpRequests/IHttpRequester.h"
#include "Common/ProxyUtils/ProxyUtils.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Obfuscation/ICipherException.h"

#include <gtest/gtest.h>

class TestProxyUtils : public LogOffInitializedTests
{
};

// getCurrentProxy tests
TEST_F(TestProxyUtils, getCurrentProxyReturnsProxyDetails)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "localhost");
    ASSERT_EQ(credentials, obfuscatedCreds);
}

TEST_F(TestProxyUtils, getCurrentProxyStillReturnsProxyIfMissingCreds)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string content = R"({"proxy":"localhost"})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "localhost");
    ASSERT_EQ(credentials, "");
}

TEST_F(TestProxyUtils, getCurrentProxyThrowsOnInvalidJson)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string content = R"({"proxy":"localhost", not json})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));
    EXPECT_THROW({
        try
        {
            Common::ProxyUtils::getCurrentProxy();
        }
        catch(const std::runtime_error& ex)
        {
            EXPECT_THAT(ex.what(), ::testing::HasSubstr("Could not parse current proxy file:"));
            throw;
        }
    }, std::runtime_error);
}

TEST_F(TestProxyUtils, getCurrentProxyThrowsOnUnreadableFile)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Throw(IFileSystemException("Cannot read file")));
    Tests::replaceFileSystem(std::move(filesystemMock));
    EXPECT_THROW({
        try
        {
            Common::ProxyUtils::getCurrentProxy();
        }
        catch(const std::runtime_error& ex)
        {
            EXPECT_THAT(ex.what(), ::testing::HasSubstr("Could not parse current proxy file:"));
            throw;
        }
    }, std::runtime_error);
}

TEST_F(TestProxyUtils, getCurrentProxyReturnsEmptyDetailsOnMissingFile)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(filesystemMock));
    auto [proxy, creds] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(creds, "");
}

TEST_F(TestProxyUtils, getCurrentProxyIgnoresExtraJsonFields)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"notproxy":"astring", "proxy":"localhost","field2":123,"credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "localhost");
    ASSERT_EQ(credentials, obfuscatedCreds);
}

TEST_F(TestProxyUtils, getCurrentProxyHandlesMissingProxyField)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"notproxy":"astring", "field2":123,"credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(credentials, "");
}

TEST_F(TestProxyUtils, getCurrentProxyDoesNotReturnAnyStringIfOnlyCredsInJson)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string content = R"({"credentials":"password"})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(credentials, "");
}

TEST_F(TestProxyUtils, getCurrentProxyReturnsEmptyValusForEmptyJson)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string content = R"({})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(credentials, "");
}

TEST_F(TestProxyUtils, getCurrentProxyReturnsEmptyValuesForEmptyFile)
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string content = "";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(credentials, "");
}


// deobfuscateProxyCreds tests

TEST_F(TestProxyUtils, deobfuscateProxyCreds)
{
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    auto [username, password] = Common::ProxyUtils::deobfuscateProxyCreds(obfuscatedCreds);
    ASSERT_EQ(username, "username");
    ASSERT_EQ(password, "password");
}

TEST_F(TestProxyUtils, deobfuscateProxyCredsThrowsOnInvalidInput)
{
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSbbbbbbbbbbbbbbbSslIIGV5rUw";
    EXPECT_THROW(Common::ProxyUtils::deobfuscateProxyCreds(obfuscatedCreds), Common::Obfuscation::ICipherException);
}

TEST_F(TestProxyUtils, deobfuscateProxyCredsReturnsEmptyCredsOnEmptyInput)
{
    auto [username, password] = Common::ProxyUtils::deobfuscateProxyCreds("");
    ASSERT_EQ(username, "");
    ASSERT_EQ(password, "");
}

//  updateHttpRequestWithProxyInfo tests
TEST_F(TestProxyUtils, updateHttpRequestWithProxyInfoWithOnlyAddress)
{
    Common::HttpRequests::RequestConfig request;
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string content = R"({"proxy":"localhost"})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));

    bool hasProxy = Common::ProxyUtils::updateHttpRequestWithProxyInfo(request);
    ASSERT_TRUE(hasProxy);
    ASSERT_EQ(request.proxy, "localhost");
    ASSERT_FALSE(request.proxyUsername.has_value());
    ASSERT_FALSE(request.proxyPassword.has_value());
}

TEST_F(TestProxyUtils, updateHttpRequestWithProxyInfoWithOnlyAddressAndCreds)
{
    Common::HttpRequests::RequestConfig request;
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));

    bool hasProxy = Common::ProxyUtils::updateHttpRequestWithProxyInfo(request);
    ASSERT_TRUE(hasProxy);
    ASSERT_EQ(request.proxy, "localhost");
    ASSERT_EQ(request.proxyUsername, "username");
    ASSERT_EQ(request.proxyPassword, "password");
}

TEST_F(TestProxyUtils, updateHttpRequestWithProxyHandlesinvalidCreds)
{
    Common::HttpRequests::RequestConfig request;
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));

    bool hasProxy = Common::ProxyUtils::updateHttpRequestWithProxyInfo(request);
    ASSERT_FALSE(hasProxy);
    ASSERT_FALSE(request.proxy.has_value());
    ASSERT_FALSE(request.proxyUsername.has_value());
    ASSERT_FALSE(request.proxyPassword.has_value());
}

TEST_F(TestProxyUtils, updateHttpRequestWithProxyHandlesEmptyFile)
{
    Common::HttpRequests::RequestConfig request;
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string content = R"({})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(filesystemMock));

    bool hasProxy = Common::ProxyUtils::updateHttpRequestWithProxyInfo(request);
    ASSERT_FALSE(hasProxy);
    ASSERT_FALSE(request.proxy.has_value());
    ASSERT_FALSE(request.proxyUsername.has_value());
    ASSERT_FALSE(request.proxyPassword.has_value());
}