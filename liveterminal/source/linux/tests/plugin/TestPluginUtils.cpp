/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/pluginimpl/PluginUtils.h>
#include <Common/FileSystem/IFileSystem.h>

#include <Common/Helpers/TempDir.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>

class TestPluginUtils: public LogOffInitializedTests{};

TEST_F(TestPluginUtils, testgetHostnameFromUrlstripsOffHTTPS)
{
    std::string url = "https://url.eng.sophos/blah";
    std::string hostname = Plugin::PluginUtils::getHostnameFromUrl(url);
    ASSERT_EQ(hostname,"url.eng.sophos");
}

TEST_F(TestPluginUtils, testgetHostnameFromUrlstripsOffHTTP)
{
    std::string url = "http://url.eng.sophos/stuff";
    std::string hostname = Plugin::PluginUtils::getHostnameFromUrl(url);
    ASSERT_EQ(hostname,"url.eng.sophos");
}

TEST_F(TestPluginUtils, testgetHostnameFromUrlstripsOffFilepath)
{
    std::string url = "url.eng.sophos/stuff/blah";
    std::string hostname = Plugin::PluginUtils::getHostnameFromUrl(url);
    ASSERT_EQ(hostname,"url.eng.sophos");
}

TEST_F(TestPluginUtils, readCurrentProxyInfo) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();
    std::string content = R"({"proxy":"localhost","credentials":"password"})";
    m_tempDir->createFile("current_proxy", content);
    std::string filepath = m_tempDir->absPath("current_proxy");

    auto fileSystem = Common::FileSystem::fileSystem();
    auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);
    EXPECT_EQ(proxy,"localhost");
    EXPECT_EQ(credentials,"password");
}

TEST_F(TestPluginUtils, readCurrentProxyInfoHandlesInvalidJson) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();
    std::string content = R"({"proxy":"localhost",})";
    m_tempDir->createFile("current_proxy", content);
    std::string filepath = m_tempDir->absPath("current_proxy");

    auto fileSystem = Common::FileSystem::fileSystem();
    auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);
    EXPECT_EQ(proxy,"");
    EXPECT_EQ(credentials,"");
}
TEST_F(TestPluginUtils, readCurrentProxyInfoHandlesInvalidFilepath) // NOLINT
{
    std::string filepath = "/file/does/not/exist";

    auto fileSystem = Common::FileSystem::fileSystem();
    auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);
    EXPECT_EQ(proxy,"");
    EXPECT_EQ(credentials,"");
}

TEST_F(TestPluginUtils, readCurrentProxyInfoHandlesIrrelevantJsonFields) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();
    std::string content = R"({"notproxy":"localhost","credentials":"password"})";
    m_tempDir->createFile("current_proxy", content);
    std::string filepath = m_tempDir->absPath("current_proxy");

    auto fileSystem = Common::FileSystem::fileSystem();
    auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);
    EXPECT_EQ(proxy,"");
    EXPECT_EQ(credentials,"");
}

TEST_F(TestPluginUtils, readCurrentProxyInfoHandlesIrrelevantJsonFieldsWhenProxyFieldIsCorrect) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();
    std::string content = R"({"proxy":"localhost","credentials":"password","proxystuff":"localhost2"})";
    m_tempDir->createFile("current_proxy", content);
    std::string filepath = m_tempDir->absPath("current_proxy");

    auto fileSystem = Common::FileSystem::fileSystem();
    auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);
    EXPECT_EQ(proxy,"localhost");
    EXPECT_EQ(credentials,"password");
}

TEST_F(TestPluginUtils, readCurrentProxyInfoReturnsNoProxyIfTwoProxyFields) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();
    std::string content = R"({proxy":"localhost","credentials":"password","proxy":"localhost2"})";
    m_tempDir->createFile("current_proxy", content);
    std::string filepath = m_tempDir->absPath("current_proxy");

    auto fileSystem = Common::FileSystem::fileSystem();
    auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);
    EXPECT_EQ(proxy,"");
    EXPECT_EQ(credentials,"");
}

TEST_F(TestPluginUtils, readCurrentProxyInfoProxyIsStillSavedIfCredentialsAreEmpty) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();
    std::string content = R"({"proxy":"localhost"})";
    m_tempDir->createFile("current_proxy", content);
    std::string filepath = m_tempDir->absPath("current_proxy");

    auto fileSystem = Common::FileSystem::fileSystem();
    auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);
    EXPECT_EQ(proxy,"localhost");
    EXPECT_EQ(credentials,"");
}

TEST_F(TestPluginUtils, readCurrentProxyInfoCredentialsAreNotStoreIfProxyIsEmpty) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();
    std::string content = R"({"credentials":"password"})";
    m_tempDir->createFile("current_proxy", content);
    std::string filepath = m_tempDir->absPath("current_proxy");

    auto fileSystem = Common::FileSystem::fileSystem();
    auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);
    EXPECT_EQ(proxy,"");
    EXPECT_EQ(credentials,"");
}

TEST_F(TestPluginUtils, readCurrentProxyInfoEmptyJsonIshandledCorrectly) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();

    m_tempDir->createFile("current_proxy", "");
    std::string filepath = m_tempDir->absPath("current_proxy");

    auto fileSystem = Common::FileSystem::fileSystem();
    auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);
    EXPECT_EQ(proxy,"");
    EXPECT_EQ(credentials,"");
}

TEST_F(TestPluginUtils, readKeyValueFromConfigNoConfigIshandledCorrectly) // NOLINT
{
    std::string filepath = "/opt/sophos-spl/base/etc/sophosspl/mcs.config";
    std::optional<std::string> endpointId = Plugin::PluginUtils::readKeyValueFromConfig("MCSID", filepath);
    EXPECT_FALSE(endpointId);
}

TEST_F(TestPluginUtils, readKeyValueFromConfigReturnsCorrectly) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();

    m_tempDir->createFile("mcs.config", "MCSID=exampleId");
    std::string filepath = m_tempDir->absPath("mcs.config");

    std::optional<std::string> endpointId = Plugin::PluginUtils::readKeyValueFromConfig("MCSID", filepath);
    EXPECT_EQ(endpointId.value(),"exampleId");
}

TEST_F(TestPluginUtils, readKeyValueFromConfigReturnsNothingWhenNotKeyNotPresent) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();
    m_tempDir->createFile("mcs.config", "customerId=exampleId");
    std::string filepath = m_tempDir->absPath("mcs.config");

    std::optional<std::string> endpointId = Plugin::PluginUtils::readKeyValueFromConfig("MCSID", filepath);
    EXPECT_FALSE(endpointId);
}

TEST_F(TestPluginUtils, readKeyValueFromConfigReturnsNothingWhenNoEqualsSeparatingKeyAndValue) // NOLINT
{
    auto m_tempDir = Tests::TempDir::makeTempDir();
    m_tempDir->createFile("mcs.config", "MCSIDexampleId");
    std::string filepath = m_tempDir->absPath("mcs.config");

    std::optional<std::string> endpointId = Plugin::PluginUtils::readKeyValueFromConfig("MCSID", filepath);
    EXPECT_FALSE(endpointId);
}


