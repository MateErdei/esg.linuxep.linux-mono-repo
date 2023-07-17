// Copyright 2023 Sophos All rights reserved.

#include "SulDownloader/suldownloaderdata/ConfigurationDataUtil.h"

#include <gtest/gtest.h>

using namespace Common::Policy;
using namespace SulDownloader::suldownloaderdata;

static Common::Policy::UpdateSettings getValidUpdateSettings()
{
    Common::Policy::UpdateSettings validSettings;
    validSettings.setSophosLocationURLs({"http://really_sophos.info"});
    validSettings.setCredentials({"username", "password"});
    validSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    validSettings.setFeatures({"CORE"});
    return validSettings;
}


//checkIfShouldForceUpdate
//esm
TEST(TestConfigurationDataUtils, returnFalseIfESMVersionIsSame)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setEsmVersion(ESMVersion("name", "token"));
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setEsmVersion(ESMVersion("name", "token"));
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnFalseIfESMVersionIsntSet)
{
    auto newSettings = getValidUpdateSettings();
    auto previousSettings = getValidUpdateSettings();
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnTrueIfESMVersionChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setEsmVersion(ESMVersion("name", "token"));
    auto previousSettings = getValidUpdateSettings();
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnTrueIfOneFieldForESMVersionChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setEsmVersion(ESMVersion("name", "token"));
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setEsmVersion(ESMVersion("name", "oldtoken"));
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}


//primary subscription
TEST(TestConfigurationDataUtils, returnsFalseIfPrimarySubDoesntChange)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnFalseIfOnlyBaseChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGID", "BaseOther", "tag", "fixed"});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnTrueIfRigidVersionChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGIDOther", "Base", "tag", "fixed"});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnTrueIfTagChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGID", "Base", "tagother", "fixed"});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnTrueIfFixedChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixedother"});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}


//product subscription
TEST(TestConfigurationDataUtils, returnsFalseIfProductSubDoesntChange)
{
    ProductSubscription productSubscription("RIGID", "Base", "tag", "fixed");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({productSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({productSubscription});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnFalseIfOnlyProductBaseChanges)
{
    ProductSubscription newProductSubscription("RIGID", "Base", "tag", "fixed");
    ProductSubscription previousProductSubscription("RIGID", "BaseOther", "tag", "fixed");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({newProductSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({previousProductSubscription});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnFalseIfProductRigidVersionChanges)
{
    ProductSubscription newProductSubscription("RIGID", "Base", "tag", "fixed");
    ProductSubscription previousProductSubscription("RIGIDOther", "Base", "tag", "fixed");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({newProductSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({previousProductSubscription});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnTrueIfProductTagAndRigidChanges)
{
    ProductSubscription newProductSubscription("RIGID", "Base", "tag", "fixed");
    ProductSubscription previousProductSubscription("RIGID", "Base", "tagother", "fixed");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({newProductSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({previousProductSubscription});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST(TestConfigurationDataUtils, returnTrueIfProductFixedChanges)
{
    ProductSubscription newProductSubscription("RIGID", "Base", "tag", "fixed");
    ProductSubscription previousProductSubscription("RIGID", "Base", "tag", "fixedother");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({newProductSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({previousProductSubscription});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}