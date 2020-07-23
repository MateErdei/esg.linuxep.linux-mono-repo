/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/CommsComponent/CommsConfig.h>
#include <tests/Common/Helpers/TempDir.h>
#include <gtest/gtest.h>
TEST(TestCommsConfig, readCurrentProxyInfo) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"proxy":"localhost","credentials":"password"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"localhost");
EXPECT_EQ(credentials,"password");
}

TEST(TestCommsConfig, readCurrentProxyInfoHandlesInvalidJson) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"proxy":"localhost",})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}
TEST(TestCommsConfig, readCurrentProxyInfoHandlesInvalidFilepath) // NOLINT
{
std::string filepath = "/file/does/not/exist";

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}

TEST(TestCommsConfig, readCurrentProxyInfoHandlesIrrelevantJsonFields) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"notproxy":"localhost","credentials":"password"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}

TEST(TestCommsConfig, readCurrentProxyInfoHandlesIrrelevantJsonFieldsWhenProxyFieldIsCorrect) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"proxy":"localhost","credentials":"password","proxystuff":"localhost2"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"localhost");
EXPECT_EQ(credentials,"password");
}

TEST(TestCommsConfig, readCurrentProxyInfoReturnsNoProxyIfTwoProxyFields) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({proxy":"localhost","credentials":"password","proxy":"localhost2"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}

TEST(TestCommsConfig, readCurrentProxyInfoProxyIsStillSavedIfCredentialsAreEmpty) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"proxy":"localhost"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"localhost");
EXPECT_EQ(credentials,"");
}

TEST(TestCommsConfig, readCurrentProxyInfoCredentialsAreNotStoreIfProxyIsEmpty) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"credentials":"password"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}

TEST(TestCommsConfig, readCurrentProxyInfoEmptyJsonIshandledCorrectly) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();

m_tempDir->createFile("current_proxy", "");
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}
