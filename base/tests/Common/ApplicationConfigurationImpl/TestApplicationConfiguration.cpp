/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************///

#include <Common/ApplicationConfigurationImpl/ApplicationConfiguration.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>

#include <stdlib.h>

namespace
{
    constexpr char pluginRegRelPath[] = "base/pluginRegistry";
    constexpr char baseBinRelPath[] = "base/bin";
}  //namespace

class ApplicationConfigurationTests : public ::testing::Test
{
public:
    ApplicationConfigurationTests()
    {
        m_mockFileSystem = new StrictMock<MockFileSystem>(false);  //Cleaned up by unique_ptr below
        std::unique_ptr<MockFileSystem> mockFileSystemUniquePtr(m_mockFileSystem);
        Tests::replaceFileSystem(std::move(mockFileSystemUniquePtr));
    }

    ~ApplicationConfigurationTests() override
    {
        Tests::restoreFileSystem();
        unsetenv("SOPHOS_INSTALL");
    }

    MockFileSystem* m_mockFileSystem;
};

TEST_F(ApplicationConfigurationTests, SophosInstallLocationFoundFromEnvironmentVariable) //NOLINT
{
    std::string envInstallLocation = "/Installed/Here";
    setenv("SOPHOS_INSTALL", "/Installed/Here", 0);
    Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
    std::string installLocation = applicationConfiguration.getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);
    ASSERT_EQ(installLocation, envInstallLocation);
}

TEST_F(ApplicationConfigurationTests, SophosInstallLocationFoundFromExecutableInBaseBin) //NOLINT
{
    std::string basePath("/opt/non-default-install/sophos-spl");
    std::string exePath("base/bin/dummy.exe");
    std::string fullPath = Common::FileSystem::join(basePath, exePath);

    EXPECT_CALL(*m_mockFileSystem, readlink("/proc/self/exe")).WillOnce(Return(fullPath));

    std::string binDirName = Common::FileSystem::dirName(fullPath);
    std::string baseDirName = Common::FileSystem::dirName(binDirName);
    std::string sophosDirName = Common::FileSystem::dirName(baseDirName);

    std::string testPath = Common::FileSystem::join(binDirName, pluginRegRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(false));
    testPath = Common::FileSystem::join(baseDirName, pluginRegRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(false));
    testPath = Common::FileSystem::join(sophosDirName, pluginRegRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(true));
    testPath = Common::FileSystem::join(sophosDirName, baseBinRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(true));

    Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
    std::string installLocation = applicationConfiguration.getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);

    ASSERT_EQ(installLocation, basePath);
}

TEST_F(ApplicationConfigurationTests, SophosInstallLocationFoundFromExecutableInBin) //NOLINT
{
    std::string basePath("/opt/non-default-install/sophos-spl");
    std::string exePath("bin/dummy.exe");
    std::string fullPath = Common::FileSystem::join(basePath, exePath);

    EXPECT_CALL(*m_mockFileSystem, readlink("/proc/self/exe")).WillOnce(Return(fullPath));

    std::string binDirName = Common::FileSystem::dirName(fullPath);
    std::string sophosDirName = Common::FileSystem::dirName(binDirName);

    std::string testPath = Common::FileSystem::join(binDirName, pluginRegRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(false));
    testPath = Common::FileSystem::join(sophosDirName, pluginRegRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(true));
    testPath = Common::FileSystem::join(sophosDirName, baseBinRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(true));
    Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
    std::string installLocation = applicationConfiguration.getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);

    ASSERT_EQ(installLocation, basePath);
}

TEST_F(ApplicationConfigurationTests, SophosInstallLocationFoundFromPluginFolder) //NOLINT
{
    std::string basePath("/opt/non-default-install/sophos-spl");
    std::string exePath("plugins/DummyPlugin/bin/DummyPlugin.exe");
    std::string fullPath = Common::FileSystem::join(basePath, exePath);

    EXPECT_CALL(*m_mockFileSystem, readlink("/proc/self/exe")).WillOnce(Return(fullPath));
    std::string pluginBinDirName = Common::FileSystem::dirName(fullPath);
    std::string pluginNameDirName = Common::FileSystem::dirName(pluginBinDirName);
    std::string pluginsDirName = Common::FileSystem::dirName(pluginNameDirName);
    std::string sophosDirName = Common::FileSystem::dirName(pluginsDirName);

    std::string testPath = Common::FileSystem::join(pluginBinDirName, pluginRegRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(false));
    testPath = Common::FileSystem::join(pluginNameDirName, pluginRegRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(false));
    testPath = Common::FileSystem::join(pluginsDirName, pluginRegRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(false));
    testPath = Common::FileSystem::join(sophosDirName, pluginRegRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(true));
    testPath = Common::FileSystem::join(sophosDirName, baseBinRelPath);
    EXPECT_CALL(*m_mockFileSystem, exists(testPath)).WillOnce(Return(true));

    Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
    std::string installLocation = applicationConfiguration.getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);

    ASSERT_EQ(installLocation, basePath);
}

TEST_F(ApplicationConfigurationTests, SophosInstallLocationReturnsDefaultLocationIfExecutablePathReturnsEmpty) //NOLINT
{
    std::string defaultInstallLocation = Common::ApplicationConfigurationImpl::DefaultInstallLocation;

    EXPECT_CALL(*m_mockFileSystem, readlink("/proc/self/exe")).WillOnce(Return(""));
    Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
    std::string installLocation = applicationConfiguration.getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);

    ASSERT_EQ(installLocation, defaultInstallLocation);
}


TEST_F(ApplicationConfigurationTests, SophosInstallLocationReturnsDefaultLocationIfExpectedFoldersCantBeFoundAndNoEnvPathSet) //NOLINT
{
    std::string path = "/Not/A/sophos-spl/Install/Path";

    std::string defaultInstallLocation = Common::ApplicationConfigurationImpl::DefaultInstallLocation;

    EXPECT_CALL(*m_mockFileSystem, readlink("/proc/self/exe")).WillOnce(Return(path));
    EXPECT_CALL(*m_mockFileSystem, exists(_)).WillRepeatedly(Return(false));
    Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
    std::string installLocation = applicationConfiguration.getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);

    ASSERT_EQ(installLocation, defaultInstallLocation);
}