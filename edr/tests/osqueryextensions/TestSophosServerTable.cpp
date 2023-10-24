// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "MockQueryContext.h"

#include "osqueryextensions/SophosServerTable.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/TempDir.h"
#else
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/TempDir.h"
#endif

#include <gtest/gtest.h>

class TestSophosServerTable : public LogOffInitializedTests
{
protected:
    TestSophosServerTable() :
        expectedResults_{ {
            { "endpoint_id", "" },
            { "customer_id", "" },
            { "installed_versions", "[]" },
        } }
    {
    }

    TableRows expectedResults_;
    std::unique_ptr<MockFileSystem> mockFileSystem_ = std::make_unique<MockFileSystem>();

    const std::string installPath_ = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    const std::string mcsConfigPath_ =
        Common::ApplicationConfiguration::applicationPathManager().getMcsConfigFilePath();
    const std::string mcsPolicyConfigPath_ =
        Common::FileSystem::join(installPath_, "base/etc/sophosspl/mcs_policy.config");
    const std::string baseVersionIniPath_ =
        Common::ApplicationConfiguration::applicationPathManager().getVersionFilePath();
    const std::string pluginsDirectory_ = Common::FileSystem::join(installPath_, "plugins");
    const std::string edrDirectory_ = Common::FileSystem::join(pluginsDirectory_, "edr");
    const std::string edrVersionIniPath_ = Common::FileSystem::join(edrDirectory_, "VERSION.ini");
};

TEST_F(TestSophosServerTable, expectNoThrowWhenGenerateCalled)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("base/etc/sophosspl/mcs.config", "MCSID=stuff");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_NO_THROW(OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, expectEmptyValuesWhenConfigFilesDoNotExist)
{
    Tests::TempDir tempDir("/tmp");
    expectedResults_[0]["endpoint_id"] = "";
    expectedResults_[0]["customer_id"] = "";

    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}
TEST_F(TestSophosServerTable, getEndpointID)
{
    Tests::TempDir tempDir("/tmp");
    expectedResults_[0]["endpoint_id"] = "stuff";
    expectedResults_[0]["customer_id"] = "";
    tempDir.createFile("base/etc/sophosspl/mcs.config", "MCSID=stuff");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, getCustomerIDAndEndpointId)
{
    Tests::TempDir tempDir("/tmp");
    expectedResults_[0]["endpoint_id"] = "stuff";
    expectedResults_[0]["customer_id"] = "customer1";
    tempDir.createFile("base/etc/sophosspl/mcs.config", "MCSID=stuff");
    tempDir.createFile("base/etc/sophosspl/mcs_policy.config", "customerId=customer1");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, getCustomerID)
{
    Tests::TempDir tempDir("/tmp");
    expectedResults_[0]["endpoint_id"] = "";
    expectedResults_[0]["customer_id"] = "customer1";
    tempDir.createFile("base/etc/sophosspl/mcs_policy.config", "customerId=customer1");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, emptyConfigFilesHandledCorrectly)
{
    Tests::TempDir tempDir("/tmp");
    expectedResults_[0]["endpoint_id"] = "";
    expectedResults_[0]["customer_id"] = "";
    tempDir.createFile("base/etc/sophosspl/mcs.config", "");
    tempDir.createFile("base/etc/sophosspl/mcs_policy.config", "");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, baseVersionIniDoesntExistCausesEmptyInstalledVersions)
{
    // Need to specify these two due to the later EXPECT_CALL on isFile
    EXPECT_CALL(*mockFileSystem_, isFile(mcsConfigPath_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem_, isFile(mcsPolicyConfigPath_)).WillRepeatedly(Return(false));

    EXPECT_CALL(*mockFileSystem_, isFile(baseVersionIniPath_)).WillRepeatedly(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    expectedResults_[0]["installed_versions"] = "[]";
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, baseVersionIniEmptyCausesEmptyInstalledVersions)
{
    ON_CALL(*mockFileSystem_, isFile(baseVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(baseVersionIniPath_)).WillRepeatedly(Return(std::vector<std::string>{}));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    expectedResults_[0]["installed_versions"] = "[]";
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, baseVersionIniDoesntHaveNameCausesEmptyInstalledVersions)
{
    ON_CALL(*mockFileSystem_, isFile(baseVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(baseVersionIniPath_))
        .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_VERSION = 1.2.3.4" }));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    expectedResults_[0]["installed_versions"] = "[]";
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, baseVersionIniDoesntHaveVersionCausesEmptyInstalledVersions)
{
    ON_CALL(*mockFileSystem_, isFile(baseVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(baseVersionIniPath_))
        .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_NAME = SPL-Base-Component" }));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    expectedResults_[0]["installed_versions"] = "[]";
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, baseVersionIniHasExpectedFieldsWithNoPluginsReturnsCorrectJson)
{
    ON_CALL(*mockFileSystem_, isFile(baseVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(baseVersionIniPath_))
        .WillRepeatedly(
            Return(std::vector<std::string>{ "PRODUCT_NAME = SPL-Base-Component", "PRODUCT_VERSION = 1.2.3.4" }));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    expectedResults_[0]["installed_versions"] = R"([{"Name":"SPL-Base-Component","installed_version":"1.2.3.4"}])";
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, pluginDirectoryExistsButHasNoVersionIniResultHasBase)
{
    // Need to specify these two due to the later EXPECT_CALL on isFile
    EXPECT_CALL(*mockFileSystem_, isFile(mcsConfigPath_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem_, isFile(mcsPolicyConfigPath_)).WillRepeatedly(Return(false));

    EXPECT_CALL(*mockFileSystem_, isFile(baseVersionIniPath_)).WillRepeatedly(Return(true));
    ON_CALL(*mockFileSystem_, readLines(baseVersionIniPath_))
        .WillByDefault(
            Return(std::vector<std::string>{ "PRODUCT_NAME = SPL-Base-Component", "PRODUCT_VERSION = 1.2.3.4" }));
    EXPECT_CALL(*mockFileSystem_, listDirectories(pluginsDirectory_))
        .WillRepeatedly(Return(std::vector<std::string>{ edrDirectory_ }));
    EXPECT_CALL(*mockFileSystem_, isFile(edrVersionIniPath_)).WillRepeatedly(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    expectedResults_[0]["installed_versions"] = R"([{"Name":"SPL-Base-Component","installed_version":"1.2.3.4"}])";
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, pluginVersionIniDoesntHaveNameResultHasBase)
{
    ON_CALL(*mockFileSystem_, isFile(baseVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(baseVersionIniPath_))
        .WillRepeatedly(
            Return(std::vector<std::string>{ "PRODUCT_NAME = SPL-Base-Component", "PRODUCT_VERSION = 1.2.3.4" }));
    ON_CALL(*mockFileSystem_, listDirectories(pluginsDirectory_))
        .WillByDefault(Return(std::vector<std::string>{ edrDirectory_ }));
    ON_CALL(*mockFileSystem_, isFile(edrVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(edrVersionIniPath_))
        .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_VERSION = 5.6.7.8" }));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    expectedResults_[0]["installed_versions"] = R"([{"Name":"SPL-Base-Component","installed_version":"1.2.3.4"}])";
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, pluginVersionIniDoesntHaveVersionResultHasBase)
{
    ON_CALL(*mockFileSystem_, isFile(baseVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(baseVersionIniPath_))
        .WillRepeatedly(
            Return(std::vector<std::string>{ "PRODUCT_NAME = SPL-Base-Component", "PRODUCT_VERSION = 1.2.3.4" }));
    ON_CALL(*mockFileSystem_, listDirectories(pluginsDirectory_))
        .WillByDefault(Return(std::vector<std::string>{ edrDirectory_ }));
    ON_CALL(*mockFileSystem_, isFile(edrVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(edrVersionIniPath_))
        .WillRepeatedly(
            Return(std::vector<std::string>{ "PRODUCT_NAME = SPL-Endpoint-Detection-and-Response-Plugin" }));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    expectedResults_[0]["installed_versions"] = R"([{"Name":"SPL-Base-Component","installed_version":"1.2.3.4"}])";
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, bothBaseAndPluginVersionInisAreGoodBothArePresentInJsonAndBaseComesFirst)
{
    ON_CALL(*mockFileSystem_, isFile(baseVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(baseVersionIniPath_))
        .WillRepeatedly(
            Return(std::vector<std::string>{ "PRODUCT_NAME = SPL-Base-Component", "PRODUCT_VERSION = 1.2.3.4" }));
    ON_CALL(*mockFileSystem_, listDirectories(pluginsDirectory_))
        .WillByDefault(Return(std::vector<std::string>{ edrDirectory_ }));
    ON_CALL(*mockFileSystem_, isFile(edrVersionIniPath_)).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem_, readLines(edrVersionIniPath_))
        .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_NAME = SPL-Endpoint-Detection-and-Response-Plugin",
                                                         "PRODUCT_VERSION = 5.6.7.8" }));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    expectedResults_[0]["installed_versions"] =
        R"([{"Name":"SPL-Base-Component","installed_version":"1.2.3.4"},{"Name":"SPL-Endpoint-Detection-and-Response-Plugin","installed_version":"5.6.7.8"}])";
    MockQueryContext context;
    EXPECT_EQ(expectedResults_, OsquerySDK::SophosServerTable().Generate(context));
}
