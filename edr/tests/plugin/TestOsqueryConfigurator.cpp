/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ALCPoliciesExample.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <modules/pluginimpl/OsqueryConfigurator.h>
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
    bool disableSystemAuditDAndTakeOwnershipOfNetlink() const
    {
        return OsqueryConfigurator::disableSystemAuditDAndTakeOwnershipOfNetlink();
    }

    std::string regenerateOSQueryFlagsFile(bool enableAuditEventCollection)
    {
        const std::string filepath = "anyfile";
        std::string fileContent;
        auto mockFileSystem = new ::testing::NiceMock<MockFileSystem>();
        EXPECT_CALL(*mockFileSystem, isFile(filepath)).WillOnce(Return(false));
        EXPECT_CALL(*mockFileSystem, writeFile(filepath, _))
            .WillOnce(
                Invoke([&fileContent](const std::string&, const std::string& content) { fileContent = content; }));

        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

        OsqueryConfigurator::regenerateOSQueryFlagsFile(filepath, enableAuditEventCollection);

        Tests::restoreFileSystem();
        return fileContent;
    }

private:
    bool retrieveDisableAuditFlagFromSettingsFile() const override
    {
        return m_disableSystemAuditDAndTakeOwnershipOfNetlink;
    }
};

TEST(TestOsqueryConfigurator, ALCContainsMTRFeatureShouldDetectPresenceOFMTR)
{ // NOLINT
    EXPECT_TRUE(OsqueryConfigurator::ALCContainsMTRFeature(PolicyWithMTRFeature()));
    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature(PolicyWithoutMTRFeatureButWithSubscription()));
    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature(PolicyWithoutMTRFeatureOrSubscription()));
}

TEST(TestOsqueryConfigurator, ALCContainsMTRFeatureSpecialCaseForEmptyStringShouldReturnFalse)
{ // NOLINT
    // empty string should be considered no MTR feature without any warning log
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();
    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature(""));
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("WARN Failed to parse ALC policy")));
}

TEST(TestOsqueryConfigurator, InvalidALCPolicyShouldBeConsideredToHaveNoMTRFeatureAndWarningShouldBeLogged) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();

    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature("Not even a valid policy"));
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("WARN Failed to parse ALC policy"));
}

TEST(TestOsqueryConfigurator, OsqueryConfiguratorLogsTheMTRBoundedFeature) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();
    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithMTRFeature());
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("INFO Detected MTR is enabled"));
}

TEST(TestOsqueryConfigurator, OsqueryConfiguratorLogsTheMTRBoundedFeatureWhenNotPresent) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();
    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription());
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("INFO No MTR Detected"));
}

TEST(TestOsqueryConfigurator, BeforALCPolicyIsGivenOsQueryConfiguratorShouldConsideredToBeMTRBounded) // NOLINT
{
    TestableOsqueryConfigurator disabledOption(false);
    EXPECT_FALSE(disabledOption.disableSystemAuditDAndTakeOwnershipOfNetlink());
    // true because no alc policy was given
    EXPECT_TRUE(disabledOption.MTRBoundEnabled());
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());

    TestableOsqueryConfigurator enabledOption(true);
    EXPECT_TRUE(enabledOption.disableSystemAuditDAndTakeOwnershipOfNetlink());
    // true because no alc policy was given
    EXPECT_TRUE(enabledOption.MTRBoundEnabled());
    EXPECT_FALSE(enabledOption.enableAuditDataCollection());
}

TEST(TestOsqueryConfigurator, ForALCNotContainingMTRFeatureCustomerChoiceShouldControlAuditConfiguration) // NOLINT
{
    TestableOsqueryConfigurator disabledOption(false);
    disabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription());

    EXPECT_FALSE(disabledOption.disableSystemAuditDAndTakeOwnershipOfNetlink());
    // false as the alc policy does not refer to mtr feature
    EXPECT_FALSE(disabledOption.MTRBoundEnabled());
    // audit collection is not enabled because of disableSystemAuditDAndTakeOwnershipOfNetlink set to false means system
    // auditd should be enabled.
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());

    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription());
    EXPECT_TRUE(enabledOption.disableSystemAuditDAndTakeOwnershipOfNetlink());
    // false as the alc policy does not refer to mtr feature
    EXPECT_FALSE(enabledOption.MTRBoundEnabled());
    // audit collection is enabled because of it has permission to take ownership of audit link
    EXPECT_TRUE(enabledOption.enableAuditDataCollection());
}

TEST(TestOsqueryConfigurator, ForALCContainingMTRFeatureAuditShouldNeverBeConfigured) // NOLINT
{
    TestableOsqueryConfigurator disabledOption(false);
    disabledOption.loadALCPolicy(PolicyWithMTRFeature());

    EXPECT_FALSE(disabledOption.disableSystemAuditDAndTakeOwnershipOfNetlink());
    EXPECT_TRUE(disabledOption.MTRBoundEnabled());
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());

    TestableOsqueryConfigurator enabledOption(true);
    enabledOption.loadALCPolicy(PolicyWithMTRFeature());
    EXPECT_TRUE(enabledOption.disableSystemAuditDAndTakeOwnershipOfNetlink());
    EXPECT_TRUE(enabledOption.MTRBoundEnabled());
    EXPECT_FALSE(enabledOption.enableAuditDataCollection());
}

TEST(TestOsqueryConfigurator, AuditCollectionIsDisabledForNotEnabledAuditDataCollection) // NOLINT
{
    TestableOsqueryConfigurator enabledOption(true);

    std::string osqueryFlags = enabledOption.regenerateOSQueryFlagsFile(true);

    EXPECT_THAT(osqueryFlags, ::testing::HasSubstr("--disable_audit=false"));

    TestableOsqueryConfigurator disabledOption(false);
    osqueryFlags = disabledOption.regenerateOSQueryFlagsFile(false);

    EXPECT_THAT(osqueryFlags, ::testing::HasSubstr("--disable_audit=true"));
}
