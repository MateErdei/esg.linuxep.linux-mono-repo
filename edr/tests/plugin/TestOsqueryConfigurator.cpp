/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ALCPoliciesExample.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <modules/pluginimpl/OsqueryConfigurator.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>
#include <tests/osqueryclient/MockOsqueryClient.h>

#include <gtest/gtest.h>

using namespace Plugin;
class TestableOsqueryConfigurator : public OsqueryConfigurator
{
    bool m_disableSystemAuditDAndTakeOwnershipOfNetlink;

public:
    TestableOsqueryConfigurator(
        bool disableSystemAuditDAndTakeOwnershipOfNetlink,
        bool mtrInAlcPolicy = true,
        std::optional<bool> mtrHasScheduledQueries = std::nullopt) : OsqueryConfigurator()
    {
        m_disableSystemAuditDAndTakeOwnershipOfNetlink = disableSystemAuditDAndTakeOwnershipOfNetlink;
        m_mtrInAlcPolicy = mtrInAlcPolicy;
        m_mtrHasScheduledQueries = mtrHasScheduledQueries;
    }

    bool getPresenceOfMtrInAlcPolicy() const
    {
        return OsqueryConfigurator::getPresenceOfMtrInAlcPolicy();
    }

    [[nodiscard]] static bool enableAuditDataCollectionInternal(bool disableAuditDInPluginConfig, bool mtrInAlcPolicy, std::optional<bool> mtrHasScheduledQueries)
    {
        return OsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries);
    }

    void replaceMtrMonitor(MtrMonitor mtrMonitor)
    {
        m_mtrMonitor = MtrMonitor(std::move(mtrMonitor));
    }

private:
    bool retrieveDisableAuditDFlagFromSettingsFile() const override
    {
        return m_disableSystemAuditDAndTakeOwnershipOfNetlink;
    }
};

class TestOsqueryConfigurator : public LogOffInitializedTests{};

TEST_F(TestOsqueryConfigurator, ALCContainsMTRFeatureShouldDetectPresenceOFMTR)
{ // NOLINT
    EXPECT_TRUE(OsqueryConfigurator::ALCContainsMTRFeature(PolicyWithMTRFeature()));
    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature(PolicyWithoutMTRFeatureButWithSubscription()));
    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature(PolicyWithoutMTRFeatureOrSubscription()));
}

TEST_F(TestOsqueryConfigurator, ALCContainsMTRFeatureSpecialCaseForEmptyStringShouldReturnFalse)
{ // NOLINT
    // empty string should be considered no MTR feature without any warning log
    testing::internal::CaptureStderr();
    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature(""));
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("WARN Failed to parse ALC policy")));
}

TEST_F(TestOsqueryConfigurator, InvalidALCPolicyShouldBeConsideredToHaveNoMTRFeatureAndWarningShouldBeLogged) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();

    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature("Not even a valid policy"));
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("WARN Failed to parse ALC policy"));
}

TEST_F(TestOsqueryConfigurator, OsqueryConfiguratorLogsTheMTRBoundedFeature) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();
    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithMTRFeature());
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("INFO Detected MTR is enabled"));
}

TEST_F(TestOsqueryConfigurator, OsqueryConfiguratorLogsTheMTRBoundedFeatureWhenNotPresent) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();
    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription());
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("INFO No MTR Detected"));
}

TEST_F(TestOsqueryConfigurator, BeforeALCPolicyIsGivenOsQueryConfiguratorShouldConsideredToBeMTRBounded) // NOLINT
{
    TestableOsqueryConfigurator disabledOption(false);
    // true because no alc policy was given
    EXPECT_TRUE(disabledOption.getPresenceOfMtrInAlcPolicy());
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());

    TestableOsqueryConfigurator enabledOption(true);
    // true because no alc policy was given
    EXPECT_TRUE(enabledOption.getPresenceOfMtrInAlcPolicy());
    EXPECT_FALSE(enabledOption.enableAuditDataCollection());
}

TEST_F(TestOsqueryConfigurator, ForALCNotContainingMTRFeatureCustomerChoiceShouldControlAuditConfiguration) // NOLINT
{
    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription());
    // false as the alc policy does not refer to mtr feature
    EXPECT_FALSE(enabledOption.getPresenceOfMtrInAlcPolicy());
    // audit collection is enabled because of it has permission to take ownership of audit link
    EXPECT_TRUE(enabledOption.enableAuditDataCollection());
}

TEST_F(TestOsqueryConfigurator, ForALCContainingMTRFeatureAuditShouldNeverBeConfigured) // NOLINT
{
    TestableOsqueryConfigurator disabledOption(false);
    disabledOption.loadALCPolicy(PolicyWithMTRFeature());

    EXPECT_TRUE(disabledOption.getPresenceOfMtrInAlcPolicy());
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());

    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithMTRFeature());
    EXPECT_TRUE(enabledOption.getPresenceOfMtrInAlcPolicy());
    EXPECT_FALSE(enabledOption.enableAuditDataCollection());
}

TEST_F(TestOsqueryConfigurator, enableAnddisableQueryPackRenamesQueryPack) // NOLINT
{
    std::string queryPackPath = "querypackpath";
    std::string queryPackPathDisabled = "querypackpath.DISABLED";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(queryPackPathDisabled, queryPackPath));
    EXPECT_CALL(*mockFileSystem, moveFile(queryPackPath, queryPackPathDisabled));
    TestableOsqueryConfigurator::enableQueryPack(queryPackPath);
    TestableOsqueryConfigurator::disableQueryPack(queryPackPath);
}

TEST_F(TestOsqueryConfigurator, enableAuditDataCollectionInternalReturnsExpectedValueGivenSetOfInputs) // NOLINT
{
    bool disableAuditDInPluginConfig = false;
    bool mtrInAlcPolicy = false;
    std::optional<bool> mtrHasScheduledQueries = false;
    ASSERT_FALSE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = true;
    mtrInAlcPolicy = false;
    mtrHasScheduledQueries = false;
    ASSERT_TRUE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = false;
    mtrInAlcPolicy = true;
    mtrHasScheduledQueries = false;
    ASSERT_FALSE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = true;
    mtrInAlcPolicy = true;
    mtrHasScheduledQueries = false;
    ASSERT_TRUE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = false;
    mtrInAlcPolicy = false;
    mtrHasScheduledQueries = true;
    ASSERT_FALSE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    // invalid state (but should always take netlink if mtr not in policy and auditd disabled)
    disableAuditDInPluginConfig = true;
    mtrInAlcPolicy = false;
    mtrHasScheduledQueries = true;
    ASSERT_TRUE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = false;
    mtrInAlcPolicy = true;
    mtrHasScheduledQueries = true;
    ASSERT_FALSE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = true;
    mtrInAlcPolicy = true;
    mtrHasScheduledQueries = true;
    ASSERT_FALSE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = false;
    mtrInAlcPolicy = false;
    mtrHasScheduledQueries = std::nullopt;
    ASSERT_FALSE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = true;
    mtrInAlcPolicy = false;
    mtrHasScheduledQueries = std::nullopt;
    ASSERT_TRUE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = false;
    mtrInAlcPolicy = true;
    mtrHasScheduledQueries = std::nullopt;
    ASSERT_FALSE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));

    disableAuditDInPluginConfig = true;
    mtrInAlcPolicy = true;
    mtrHasScheduledQueries = std::nullopt;
    ASSERT_FALSE(TestableOsqueryConfigurator::enableAuditDataCollectionInternal(disableAuditDInPluginConfig, mtrInAlcPolicy, mtrHasScheduledQueries));
}

TEST_F(TestOsqueryConfigurator, checkIfReconfigurationRequiredReturnsFalseIfEdrHasNetlinkAndMtrIsNeverRunningQueries) // NOLINT
{
    // start up configurator with EDR not having netlink
    TestableOsqueryConfigurator osqueryConfigurator(true, true, false);

    // setup mocks to make MTR appear to be not running queries so hasScheduledQueriesConfigured returns false.
    // Filesystem mock
    std::string socket = "/socket/path/socket.sock";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    // Mock flag file existing to get MTR osquery socket path from
    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));

    // Mock reading the flag file
    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    // Setup osquery client mock
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["query_count"] = "0";
    OsquerySDK::QueryData data;
    data.push_back(row);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));


    // Inject mock osquery client into a mtrMonitor instance
    MtrMonitor mtrMonitor(std::move(mockOsqueryClient));

    // Swap out mtrMonitor instance in osqueryConfigurator to the one which we have setup the mock with.
    osqueryConfigurator.replaceMtrMonitor(std::move(mtrMonitor));

    bool reconfigurationRequired = osqueryConfigurator.checkIfReconfigurationRequired();
    EXPECT_FALSE(reconfigurationRequired);
}

TEST_F(TestOsqueryConfigurator, checkIfReconfigurationRequiredReturnsTrueIfEdrDoesNotHaveNetlinkAndThenMtrIsNotRunningQueries) // NOLINT
{
    // start up configurator with EDR having netlink
    TestableOsqueryConfigurator osqueryConfigurator(true, true, true);

    // setup mocks to make MTR appear to be not running queries so hasScheduledQueriesConfigured returns false.
    // Filesystem mock
    std::string socket = "/socket/path/socket.sock";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    // Mock flag file existing to get MTR osquery socket path from
    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));

    // Mock reading the flag file
    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    // Setup osquery client mock
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["query_count"] = "0";
    OsquerySDK::QueryData data;
    data.push_back(row);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));

    // Inject mock osquery client into a mtrMonitor instance
    MtrMonitor mtrMonitor(std::move(mockOsqueryClient));

    // Swap out mtrMonitor instance in osqueryConfigurator to the one which we have setup the mock with.
    osqueryConfigurator.replaceMtrMonitor(std::move(mtrMonitor));

    bool reconfigurationRequired = osqueryConfigurator.checkIfReconfigurationRequired();
    EXPECT_TRUE(reconfigurationRequired);
}

TEST_F(TestOsqueryConfigurator, checkIfReconfigurationRequiredReturnsTrueIfEdrDoesNotHaveNetlinkAndMtrIsQueriedForTheFirstTimeAndIsNotRunningQueries) // NOLINT
{
    // start up configurator with EDR having netlink
    TestableOsqueryConfigurator osqueryConfigurator(true, true);

    // setup mocks to make MTR appear to be not running queries so hasScheduledQueriesConfigured returns false.
    // Filesystem mock
    std::string socket = "/socket/path/socket.sock";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    // Mock flag file existing to get MTR osquery socket path from
    EXPECT_CALL(*mockFileSystem, exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));

    // Mock reading the flag file
    std::vector<std::string> flags = { "some text", "--extensions_socket=" + socket, "--flag=value" };
    EXPECT_CALL(*mockFileSystem, readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags"))
        .WillOnce(Return(flags));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    // Setup osquery client mock
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["query_count"] = "0";
    OsquerySDK::QueryData data;
    data.push_back(row);
    EXPECT_CALL(*mockOsqueryClient, query(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status { 0, "some message" })));

    // Inject mock osquery client into a mtrMonitor instance
    MtrMonitor mtrMonitor(std::move(mockOsqueryClient));

    // Swap out mtrMonitor instance in osqueryConfigurator to the one which we have setup the mock with.
    osqueryConfigurator.replaceMtrMonitor(std::move(mtrMonitor));

    bool reconfigurationRequired = osqueryConfigurator.checkIfReconfigurationRequired();
    EXPECT_TRUE(reconfigurationRequired);
}

TEST_F(TestOsqueryConfigurator, checkIfReconfigurationRequiredReturnsFalseIfEdrDoesNotHaveNetlinkAndMtrIsQueriedForTheFirstTimeAndIsRunningQueries) // NOLINT
{
    // start up configurator with EDR having netlink
    TestableOsqueryConfigurator osqueryConfigurator(true, true);

    // setup mocks to make MTR appear to be not running queries so hasScheduledQueriesConfigured returns false.
    // Filesystem mock
    std::string socket = "/socket/path/socket.sock";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    // Mock flag file existing to get MTR osquery socket path from
    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));

    // Mock reading the flag file
    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));
    // Setup osquery client mock
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["query_count"] = "12";
    OsquerySDK::QueryData data;
    data.push_back(row);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));

    // Inject mock osquery client into a mtrMonitor instance
    MtrMonitor mtrMonitor(std::move(mockOsqueryClient));

    // Swap out mtrMonitor instance in osqueryConfigurator to the one which we have setup the mock with.
    osqueryConfigurator.replaceMtrMonitor(std::move(mtrMonitor));

    bool reconfigurationRequired = osqueryConfigurator.checkIfReconfigurationRequired();
    EXPECT_FALSE(reconfigurationRequired);
}

TEST_F(TestOsqueryConfigurator, checkIfReconfigurationRequiredReturnsTrueIfEdrHasNetlinkAndMtrStartsRunningQueries) // NOLINT
{
    // start up configurator with EDR having netlink
    TestableOsqueryConfigurator osqueryConfigurator(true, true, false);

    // setup mocks to make MTR appear to be not running queries so hasScheduledQueriesConfigured returns false.
    // Filesystem mock
    std::string socket = "/socket/path/socket.sock";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    // Mock flag file existing to get MTR osquery socket path from
    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));

    // Mock reading the flag file
    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));
    // Setup osquery client mock
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["query_count"] = "12";
    OsquerySDK::QueryData data;
    data.push_back(row);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));

    // Inject mock osquery client into a mtrMonitor instance
    MtrMonitor mtrMonitor(std::move(mockOsqueryClient));

    // Swap out mtrMonitor instance in osqueryConfigurator to the one which we have setup the mock with.
    osqueryConfigurator.replaceMtrMonitor(std::move(mtrMonitor));

    bool reconfigurationRequired = osqueryConfigurator.checkIfReconfigurationRequired();
    EXPECT_TRUE(reconfigurationRequired);
}

TEST_F(TestOsqueryConfigurator, checkIfReconfigurationRequiredReturnsFalseIfMtrNotInPolicy) // NOLINT
{
    TestableOsqueryConfigurator osqueryConfigurator(true, false);

    // setup mocks to make MTR appear to be not running queries so hasScheduledQueriesConfigured returns false.
    // Filesystem mock
    std::string socket = "/socket/path/socket.sock";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    // Mock flag file existing to get MTR osquery socket path from
    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));

    // Mock reading the flag file
    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));

    auto mockOsqueryClient = new ::testing::StrictMock<MockIOsqueryClient>();

    MtrMonitor mtrMonitor(static_cast<std::unique_ptr<osqueryclient::IOsqueryClient>>(mockOsqueryClient));

    // Swap out mtrMonitor instance in osqueryConfigurator to the one which we have setup the mock with.
    osqueryConfigurator.replaceMtrMonitor(std::move(mtrMonitor));

    bool reconfigurationRequired = osqueryConfigurator.checkIfReconfigurationRequired();
    EXPECT_FALSE(reconfigurationRequired);
}
