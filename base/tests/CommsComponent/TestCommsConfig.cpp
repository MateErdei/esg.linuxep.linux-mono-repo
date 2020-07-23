/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/CommsComponent/CommsConfig.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>
class TestCommsConfig : public LogInitializedTests
{};
TEST_F(TestCommsConfig, readCurrentProxyInfo) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"proxy":"localhost","credentials":"password"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"localhost");
EXPECT_EQ(credentials,"password");
}

TEST_F(TestCommsConfig, readCurrentProxyInfoHandlesInvalidJson) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"proxy":"localhost",})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}

TEST_F(TestCommsConfig, readCurrentProxyInfoHandlesInvalidFilepath) // NOLINT
{
std::string filepath = "/file/does/not/exist";

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}

TEST_F(TestCommsConfig, readCurrentProxyInfoHandlesIrrelevantJsonFields) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"notproxy":"localhost","credentials":"password"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}

TEST_F(TestCommsConfig, readCurrentProxyInfoHandlesIrrelevantJsonFieldsWhenProxyFieldIsCorrect) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"proxy":"localhost","credentials":"password","proxystuff":"localhost2"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"localhost");
EXPECT_EQ(credentials,"password");
}

TEST_F(TestCommsConfig, readCurrentProxyInfoReturnsNoProxyIfTwoProxyFields) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({proxy":"localhost","credentials":"password","proxy":"localhost2"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}

TEST_F(TestCommsConfig, readCurrentProxyInfoProxyIsStillSavedIfCredentialsAreEmpty) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"proxy":"localhost"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"localhost");
EXPECT_EQ(credentials,"");
}

TEST_F(TestCommsConfig, readCurrentProxyInfoCredentialsAreNotStoreIfProxyIsEmpty) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();
std::string content = R"({"credentials":"password"})";
m_tempDir->createFile("current_proxy", content);
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}

TEST_F(TestCommsConfig, readCurrentProxyInfoEmptyJsonIshandledCorrectly) // NOLINT
{
auto m_tempDir = Tests::TempDir::makeTempDir();

m_tempDir->createFile("current_proxy", "");
std::string filepath = m_tempDir->absPath("current_proxy");

auto [proxy,credentials] = CommsComponent::CommsConfig::readCurrentProxyInfo(filepath);
EXPECT_EQ(proxy,"");
EXPECT_EQ(credentials,"");
}
