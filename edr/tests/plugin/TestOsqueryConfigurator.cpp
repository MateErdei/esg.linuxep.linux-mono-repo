/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ALCPoliciesExample.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/TempDir.h>
#include <modules/pluginimpl/OsqueryConfigurator.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

#include <gtest/gtest.h>

using namespace Plugin;
class TestableOsqueryConfigurator : public OsqueryConfigurator
{
    bool m_disableSystemAuditDAndTakeOwnershipOfNetlink;

public:
    TestableOsqueryConfigurator(bool disableSystemAuditDAndTakeOwnershipOfNetlink) : OsqueryConfigurator()
    {
        m_disableSystemAuditDAndTakeOwnershipOfNetlink = disableSystemAuditDAndTakeOwnershipOfNetlink;
    }
    bool MTRBoundEnabled() const
    {
        return OsqueryConfigurator::MTRBoundEnabled();
    }

private:
    bool retrieveDisableAuditFlagFromSettingsFile() const override
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
    EXPECT_TRUE(disabledOption.MTRBoundEnabled());
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());

    TestableOsqueryConfigurator enabledOption(true);
    // true because no alc policy was given
    EXPECT_TRUE(enabledOption.MTRBoundEnabled());
    EXPECT_FALSE(enabledOption.enableAuditDataCollection());
}

TEST_F(TestOsqueryConfigurator, ForALCNotContainingMTRFeatureCustomerChoiceShouldControlAuditConfiguration) // NOLINT
{
    TestableOsqueryConfigurator disabledOption(false);
    disabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription());

    // false as the alc policy does not refer to mtr feature
    EXPECT_FALSE(disabledOption.MTRBoundEnabled());
    // audit collection is not enabled because of disableSystemAuditDAndTakeOwnershipOfNetlink set to false means system
    // auditd should be enabled.
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());

    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription());
    // false as the alc policy does not refer to mtr feature
    EXPECT_FALSE(enabledOption.MTRBoundEnabled());
    // audit collection is enabled because of it has permission to take ownership of audit link
    EXPECT_TRUE(enabledOption.enableAuditDataCollection());
}

TEST_F(TestOsqueryConfigurator, ForALCContainingMTRFeatureAuditShouldNeverBeConfigured) // NOLINT
{
    TestableOsqueryConfigurator disabledOption(false);
    disabledOption.loadALCPolicy(PolicyWithMTRFeature());

    EXPECT_TRUE(disabledOption.MTRBoundEnabled());
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());

    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithMTRFeature());
    EXPECT_TRUE(enabledOption.MTRBoundEnabled());
    EXPECT_FALSE(enabledOption.enableAuditDataCollection());
}

TEST_F(TestOsqueryConfigurator, enableAnddisableQueryPackRenamesQueryPack) // NOLINT
{
    std::string queryPackPath = "querypackpath";
    std::string queryPackPathDisabled = "querypackpath.DISABLED";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(queryPackPathDisabled,queryPackPath));
    EXPECT_CALL(*mockFileSystem, moveFile(queryPackPath,queryPackPathDisabled));
    TestableOsqueryConfigurator::enableQueryPack(queryPackPath);
    TestableOsqueryConfigurator::disableQueryPack(queryPackPath);

}
