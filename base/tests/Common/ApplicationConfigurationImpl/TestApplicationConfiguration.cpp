/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************///

#include <Common/ApplicationConfigurationImpl/ApplicationConfiguration.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>

#include <stdlib.h>

class ApplicationConfigurationTests : public ::testing::Test
{
public:
    ~ApplicationConfigurationTests() override
    {
        unsetenv("SOPHOS_INSTALL");
    }
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

    std::unique_ptr<MockFileSystem> mockFileSystem(new StrictMock<MockFileSystem>());
    EXPECT_CALL(*mockFileSystem, readlink("/proc/self/exe")).WillOnce(Return(fullPath));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
    std::string installLocation = applicationConfiguration.getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);

    ASSERT_EQ(installLocation, basePath);
}

TEST_F(ApplicationConfigurationTests, SophosInstallLocationFoundFromExecutableInBin) //NOLINT
{
    std::string basePath("/opt/non-default-install/sophos-spl");
    std::string exePath("bin/dummy.exe");
    std::string fullPath = Common::FileSystem::join(basePath, exePath);

    std::unique_ptr<MockFileSystem> mockFileSystem(new StrictMock<MockFileSystem>());
    EXPECT_CALL(*mockFileSystem, readlink("/proc/self/exe")).WillOnce(Return(fullPath));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
    std::string installLocation = applicationConfiguration.getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);

    ASSERT_EQ(installLocation, basePath);
}

TEST_F(ApplicationConfigurationTests, SophosInstallLocationReturnsDefaultLocationIfExecutableCantBeFoundAndNoEnvPathSet) //NOLINT
{
    std::string defaultInstallLocation = Common::ApplicationConfigurationImpl::DefaultInstallLocation;

    std::unique_ptr<MockFileSystem> mockFileSystem(new StrictMock<MockFileSystem>());
    EXPECT_CALL(*mockFileSystem, readlink("/proc/self/exe")).WillOnce(Return(""));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
    std::string installLocation = applicationConfiguration.getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);

    ASSERT_EQ(installLocation, defaultInstallLocation);
}