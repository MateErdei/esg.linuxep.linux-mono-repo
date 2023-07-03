// Copyright 2023 Sophos All rights reserved.

#include "modules/Common/Policy/UpdateSettings.h"

#include "modules/Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include "TestUpdateSettingsBase.h"

using namespace Common::Policy;

namespace
{
    class TestUpdateSettings : public TestUpdateSettingsBase
    {
    };
}

TEST_F(TestUpdateSettings, constructionDoesNotThrow)
{
    EXPECT_NO_THROW(UpdateSettings settings);
}

TEST_F(TestUpdateSettings, initialSettingsAreInvalid)
{
    UpdateSettings settings;
    EXPECT_FALSE(settings.verifySettingsAreValid());
    EXPECT_FALSE(settings.isVerified());
}

TEST_F(TestUpdateSettings, validSettingsAreValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings validSettings = getValidUpdateSettings();
    EXPECT_TRUE(validSettings.verifySettingsAreValid());
    EXPECT_TRUE(validSettings.isVerified()); // Cross check the isVerified method
}

TEST_F(TestUpdateSettings, emptyPrimarySubscriptionIsInvalid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setPrimarySubscription({});
    EXPECT_FALSE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, rigidNameOnlyPrimarySubscriptionIsInvalid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setPrimarySubscription({ "rigidname", "", "", "" });
    EXPECT_FALSE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, PrimarySubscriptionWithoutRigidNameIsInvalid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setPrimarySubscription({ "", "baseversion", "RECOMMENDED", "None" });
    EXPECT_FALSE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, PrimarySubscriptionWithoutTagOrFixedIsInvalid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setPrimarySubscription({ "rigidname", "baseversion", "", "" });
    EXPECT_FALSE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, ProductWithoutTagOrFixedIsInvalid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setProductsSubscription(
        { ProductSubscription("rigidname", "", "RECOMMENDED", ""),
          ProductSubscription("rigidname", "", "", "")
        });
    EXPECT_FALSE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, NoFeaturesIsInvalid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setFeatures({});
    EXPECT_FALSE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, MissingCoreFeatureIsInvalid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setFeatures({ "SAV", "MDR", "SENSOR" });
    EXPECT_FALSE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, PrimaryWithTagIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setPrimarySubscription({ "rigidname", "baseversion", "RECOMMENDED", "" });
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, PrimaryWithoutBaseVersionIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setPrimarySubscription({ "rigidname", "", "RECOMMENDED", "" });
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, PrimaryWithTagAndFixedVersionIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setPrimarySubscription({ "rigidname", "", "RECOMMENDED", "9.1" });
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, PrimaryWithOnlyFixedVersionIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setPrimarySubscription({ "rigidname", "", "", "9.1" });
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, FeatureListIncludingCoreIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setFeatures({ "CORE", "MDR" });
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, FeatureListOnlyCoreIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setFeatures({ "CORE" });
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, MultipleProductSubscriptionIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setProductsSubscription({ { "p1", "", "RECOMMENDED", "" }, { "p2", "", "", "9.1" } });
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, OnlyPrimaryProductSubscriptionIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    settings.setProductsSubscription({});
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, NoProxyIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    Proxy proxy;
    ASSERT_TRUE(proxy.empty());
    settings.setPolicyProxy(proxy);
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, EmptyESMIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    ESMVersion esmVersion("", "");
    settings.setEsmVersion(esmVersion);
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, PopulatedESMIsValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    ESMVersion esmVersion("esmname", "esmtoken");
    settings.setEsmVersion(esmVersion);
    EXPECT_TRUE(settings.verifySettingsAreValid());
}

TEST_F(TestUpdateSettings, PartiallyPopulatedESMIsInvalid)
{
    setupFileSystemAndGetMock();
    UpdateSettings settings = getValidUpdateSettings();
    ESMVersion esmVersion1("esmname", "");
    settings.setEsmVersion(esmVersion1);
    EXPECT_FALSE(settings.verifySettingsAreValid());

    ESMVersion esmVersion2("", "esmtoken");
    settings.setEsmVersion(esmVersion2);
    EXPECT_FALSE(settings.verifySettingsAreValid());
}