/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/pluginimpl/OsqueryConfigurator.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>

#include <gtest/gtest.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

const char * PolicyWithoutMTRFeatureButWithSubscription = R"(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
    <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
    <AUConfig platform="Linux">
        <sophos_address address="http://es-web.sophos.com/update"/>
        <primary_location>
            <server BandwidthLimit="0" AutoDial="false" Algorithm="AES256" UserPassword="XXX=" UserName="regruser" UseSophos="true" UseHttps="true" UseDelta="true" ConnectionAddress="http://dci.sophosupd.com/cloudupdate" AllowLocalConfig="false"/>
            <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
        </primary_location>
        <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="20" DetectDialUp="false"/>
        <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
        <bootstrap Location="" UsePrimaryServerAddress="true"/>
        <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
        <cloud_subscriptions>
            <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
            <subscription Id="MDR" RigidName="ServerProtectionLinux-Plugin-MDR" Tag="RECOMMENDED"/>
        </cloud_subscriptions>
        <delay_supplements enabled="false"/>
    </AUConfig>
    <Features>
        <Feature id="CORE"/>
    </Features>
    <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
    <customer id="4b4ca3ba-c144-4447-8050-6c96a7104c11"/>
</AUConfigurations>
)";

const char * PolicyWithoutMTRFeatureOrSubscription = R"(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
    <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
    <AUConfig platform="Linux">
        <sophos_address address="http://es-web.sophos.com/update"/>
        <primary_location>
            <server BandwidthLimit="0" AutoDial="false" Algorithm="AES256" UserPassword="XXX=" UserName="regruser" UseSophos="true" UseHttps="true" UseDelta="true" ConnectionAddress="http://dci.sophosupd.com/cloudupdate" AllowLocalConfig="false"/>
            <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
        </primary_location>
        <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="20" DetectDialUp="false"/>
        <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
        <bootstrap Location="" UsePrimaryServerAddress="true"/>
        <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
        <cloud_subscriptions>
            <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
            <subscription Id="EDR" RigidName="ServerProtectionLinux-Plugin-EDR" Tag="RECOMMENDED"/>
        </cloud_subscriptions>
        <delay_supplements enabled="false"/>
    </AUConfig>
    <Features>
        <Feature id="CORE"/>
    </Features>
    <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
    <customer id="4b4ca3ba-c144-4447-8050-6c96a7104c11"/>
</AUConfigurations>
)";

const char * PolicyWithMTRFeature = R"(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
    <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
    <AUConfig platform="Linux">
        <sophos_address address="http://es-web.sophos.com/update"/>
        <primary_location>
            <server BandwidthLimit="0" AutoDial="false" Algorithm="AES256" UserPassword="XXX=" UserName="regruser" UseSophos="true" UseHttps="true" UseDelta="true" ConnectionAddress="http://dci.sophosupd.com/cloudupdate" AllowLocalConfig="false"/>
            <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
        </primary_location>
        <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="20" DetectDialUp="false"/>
        <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
        <bootstrap Location="" UsePrimaryServerAddress="true"/>
        <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
        <cloud_subscriptions>
            <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
            <subscription Id="EDR" RigidName="ServerProtectionLinux-Plugin-EDR" Tag="RECOMMENDED"/>
        </cloud_subscriptions>
        <delay_supplements enabled="false"/>
    </AUConfig>
    <Features>
        <Feature id="CORE"/>
        <Feature id="MDR"/>
        <Feature id="SDU"/>
        <Feature id="LIVEQUERY"/>
    </Features>
    <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
    <customer id="4b4ca3ba-c144-4447-8050-6c96a7104c11"/>
</AUConfigurations>
)";

using namespace Plugin;
class TestableOsqueryConfigurator : public  OsqueryConfigurator
{
    bool m_disableAuditFlag;
public:
    TestableOsqueryConfigurator( bool disableAuditFlag ): OsqueryConfigurator()
    {
        m_disableAuditFlag = disableAuditFlag;
    }
    bool MTRBoundEnabled() const
    {
        return OsqueryConfigurator::MTRBoundEnabled();
    }
    bool disableAuditFlag() const
    {
        return OsqueryConfigurator::disableAuditFlag();
    }

    std::string regenerateOSQueryFlagsFile(bool enableAuditEventCollection)
    {
        const std::string filepath = "anyfile";
        std::string fileContent;
        auto mockFileSystem = new ::testing::NiceMock<MockFileSystem>();
        EXPECT_CALL(*mockFileSystem, isFile(filepath)).WillOnce(Return(false));
        EXPECT_CALL(*mockFileSystem, writeFile(filepath, _)).WillOnce(Invoke(
                [&fileContent](const std::string&, const std::string & content){fileContent = content; }));

        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});

        OsqueryConfigurator::regenerateOSQueryFlagsFile(filepath, enableAuditEventCollection);

        Tests::restoreFileSystem();
        return fileContent;
    }

private:
    bool retrieveDisableAuditFlagFromSettingsFile() const override {
        return m_disableAuditFlag;
    }

};

TEST(TestOsqueryConfigurator, ALCContainsMTRFeatureShouldDetectPresenceOFMTR) { // NOLINT
    EXPECT_TRUE(OsqueryConfigurator::ALCContainsMTRFeature(PolicyWithMTRFeature));
    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature(PolicyWithoutMTRFeatureButWithSubscription));
    EXPECT_FALSE(OsqueryConfigurator::ALCContainsMTRFeature(PolicyWithoutMTRFeatureOrSubscription));
}

TEST(TestOsqueryConfigurator, InvalidALCPolicyShouldBeConsideredToHaveNoMTRFeatureAndWarningShouldBeLogged) //NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();

    EXPECT_FALSE( OsqueryConfigurator::ALCContainsMTRFeature("Not even a valid policy") );
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("WARN Failed to parse ALC policy"));
}

TEST(TestOsqueryConfigurator, OsqueryConfiguratorLogsTheMTRBoundedFeature) //NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();
    TestableOsqueryConfigurator enabledOption(false);
    enabledOption.loadALCPolicy(PolicyWithMTRFeature);
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("INFO Detected MTR is enabled"));
}

TEST(TestOsqueryConfigurator, OsqueryConfiguratorLogsTheMTRBoundedFeatureWhenNotPresent) //NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();
    TestableOsqueryConfigurator enabledOption(false);
    enabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription);
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("INFO No MTR Detected"));
}

TEST(TestOsqueryConfigurator, BeforALCPolicyIsGivenOsQueryConfiguratorShouldConsideredToBeMTRBounded) //NOLINT
{
    TestableOsqueryConfigurator disabledOption(true);
    EXPECT_TRUE(disabledOption.disableAuditFlag());
    // true because no alc policy was given
    EXPECT_TRUE(disabledOption.MTRBoundEnabled());
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());

    TestableOsqueryConfigurator enabledOption(false);
    EXPECT_FALSE(enabledOption.disableAuditFlag());
    // true because no alc policy was given
    EXPECT_TRUE(enabledOption.MTRBoundEnabled());
    EXPECT_FALSE(enabledOption.enableAuditDataCollection());
}

TEST(TestOsqueryConfigurator, ForALCNotContainingMTRFeatureCustomerChoiceShouldControlAuditConfiguration) //NOLINT
{
    TestableOsqueryConfigurator disabledOption(true);
    disabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription);

    EXPECT_TRUE(disabledOption.disableAuditFlag());
    // false as the alc policy does not refer to mtr feature
    EXPECT_FALSE(disabledOption.MTRBoundEnabled());
    // audit collection is disabled because of disableAuditFlag
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());


    TestableOsqueryConfigurator enabledOption(false);
    enabledOption.loadALCPolicy(PolicyWithoutMTRFeatureOrSubscription);
    EXPECT_FALSE(enabledOption.disableAuditFlag());
    // false as the alc policy does not refer to mtr feature
    EXPECT_FALSE(enabledOption.MTRBoundEnabled());
    // audit collection is enabled because of Audit Flag.
    EXPECT_TRUE(enabledOption.enableAuditDataCollection());
}

TEST(TestOsqueryConfigurator, ForALCContainingMTRFeatureAuditShouldNeverBeConfigured) //NOLINT
{
    TestableOsqueryConfigurator disabledOption(true);
    disabledOption.loadALCPolicy(PolicyWithMTRFeature);

    EXPECT_TRUE(disabledOption.disableAuditFlag());
    EXPECT_TRUE(disabledOption.MTRBoundEnabled());
    EXPECT_FALSE(disabledOption.enableAuditDataCollection());


    TestableOsqueryConfigurator enabledOption(false);
    enabledOption.loadALCPolicy(PolicyWithMTRFeature);
    EXPECT_FALSE(enabledOption.disableAuditFlag());
    EXPECT_TRUE(enabledOption.MTRBoundEnabled());
    EXPECT_FALSE(enabledOption.enableAuditDataCollection());
}


TEST(TestOsqueryConfigurator, AuditCollectionIsDisabledForNotEnabledAuditDataCollection) //NOLINT
{
    TestableOsqueryConfigurator enabledOption(false);

    std::string osqueryFlags = enabledOption.regenerateOSQueryFlagsFile(true);

    EXPECT_THAT(osqueryFlags, ::testing::HasSubstr("--disable_audit=false"));

    TestableOsqueryConfigurator disabledOption(true);
    osqueryFlags = disabledOption.regenerateOSQueryFlagsFile(false);

    EXPECT_THAT(osqueryFlags, ::testing::HasSubstr("--disable_audit=true"));
}






