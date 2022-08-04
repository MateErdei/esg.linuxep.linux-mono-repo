/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "json.hpp"

#include "../Helpers/FileSystemReplaceAndRestore.h"
#include "../Helpers/LogInitializedTests.h"
#include "../Helpers/MockFileSystem.h"
#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "FileSystem/IFileSystemException.h"
#include "Obfuscation/ICipherException.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ProxyUtils/ProxyUtils.h>
#include <gtest/gtest.h>

class TestProxyUtils: public LogOffInitializedTests{};

// getCurrentProxy tests
TEST_F(TestProxyUtils, getCurrentProxyReturnsProxyDetails) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "localhost");
    ASSERT_EQ(credentials, obfuscatedCreds);
}

TEST_F(TestProxyUtils, getCurrentProxyStillReturnsProxyIfMissingCreds) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string content = R"({"proxy":"localhost"})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "localhost");
    ASSERT_EQ(credentials, "");
}

TEST_F(TestProxyUtils, getCurrentProxyThrowsOnInvalidJson) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string content = R"({"proxy":"localhost", not json})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
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

TEST_F(TestProxyUtils, getCurrentProxyThrowsOnUnreadableFile) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Throw(IFileSystemException("Cannot read file")));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
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

TEST_F(TestProxyUtils, getCurrentProxyReturnsEmptyDetailsOnMissingFile) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    auto [proxy, creds] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(creds, "");
}

TEST_F(TestProxyUtils, getCurrentProxyIgnoresExtraJsonFields) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"notproxy":"astring", "proxy":"localhost","field2":123,"credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "localhost");
    ASSERT_EQ(credentials, obfuscatedCreds);
}

TEST_F(TestProxyUtils, getCurrentProxyHandlesMissingProxyField) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"notproxy":"astring", "field2":123,"credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(credentials, "");
}

TEST_F(TestProxyUtils, getCurrentProxyDoesNotReturnAnyStringIfOnlyCredsInJson) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string content = R"({"credentials":"password"})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(credentials, "");
}

TEST_F(TestProxyUtils, getCurrentProxyReturnsEmptyValusForEmptyJson) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string content = R"({})";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(credentials, "");
}

TEST_F(TestProxyUtils, getCurrentProxyReturnsEmptyValuesForEmptyFile) // NOLINT
{
    auto currentProxyFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string content = "";
    EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(currentProxyFilePath)).WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    auto [proxy, credentials] = Common::ProxyUtils::getCurrentProxy();
    ASSERT_EQ(proxy, "");
    ASSERT_EQ(credentials, "");
}


// deobfuscateProxyCreds tests

TEST_F(TestProxyUtils, deobfuscateProxyCreds) // NOLINT
{
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    auto [username, password] = Common::ProxyUtils::deobfuscateProxyCreds(obfuscatedCreds);
    ASSERT_EQ(username, "username");
    ASSERT_EQ(password, "password");
}

TEST_F(TestProxyUtils, deobfuscateProxyCredsThrowsOnInvalidInput) // NOLINT
{
    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSbbbbbbbbbbbbbbbSslIIGV5rUw";
    EXPECT_THROW(Common::ProxyUtils::deobfuscateProxyCreds(obfuscatedCreds),  Common::Obfuscation::ICipherException);
}

TEST_F(TestProxyUtils, deobfuscateProxyCredsReturnsEmptyCredsOnEmptyInput) // NOLINT
{
    auto [username, password] = Common::ProxyUtils::deobfuscateProxyCreds("");
    ASSERT_EQ(username, "");
    ASSERT_EQ(password, "");
}