// Copyright 2023 Sophos All rights reserved.

#include "SulDownloader/suldownloaderdata/ConfigurationDataUtil.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"

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

class TestConfigurationDataUtils : public MemoryAppenderUsingTests
{
public:
    TestConfigurationDataUtils() : MemoryAppenderUsingTests("suldownloaderdata"){}
    void SetUp() override
    {
        auto mockFileSystem = std::make_unique<NiceMock<MockFileSystem>>();
        ON_CALL(*mockFileSystem, isDirectory(_)).WillByDefault(Return(true));
        Tests::replaceFileSystem(std::move(mockFileSystem));
    }
};

//checkIfShouldForceUpdate
//esm
TEST_F(TestConfigurationDataUtils, returnFalseIfESMVersionIsSame)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setEsmVersion(ESMVersion("name", "token"));
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setEsmVersion(ESMVersion("name", "token"));
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnFalseIfESMVersionIsntSet)
{
    auto newSettings = getValidUpdateSettings();
    auto previousSettings = getValidUpdateSettings();
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnTrueIfESMVersionChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setEsmVersion(ESMVersion("name", "token"));
    auto previousSettings = getValidUpdateSettings();
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnTrueIfOneFieldForESMVersionChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setEsmVersion(ESMVersion("name", "token"));
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setEsmVersion(ESMVersion("name", "oldtoken"));
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}


//primary subscription
TEST_F(TestConfigurationDataUtils, returnsFalseIfPrimarySubDoesntChange)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnFalseIfOnlyBaseChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGID", "BaseOther", "tag", "fixed"});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnTrueIfRigidVersionChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGIDOther", "Base", "tag", "fixed"});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnTrueIfTagChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGID", "Base", "tagother", "fixed"});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnTrueIfFixedChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixedother"});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}


//product subscription
TEST_F(TestConfigurationDataUtils, returnsFalseIfProductSubDoesntChange)
{
    ProductSubscription productSubscription("RIGID", "Base", "tag", "fixed");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({productSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({productSubscription});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnFalseIfOnlyProductBaseChanges)
{
    ProductSubscription newProductSubscription("RIGID", "Base", "tag", "fixed");
    ProductSubscription previousProductSubscription("RIGID", "BaseOther", "tag", "fixed");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({newProductSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({previousProductSubscription});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnFalseIfProductRigidVersionChanges)
{
    ProductSubscription newProductSubscription("RIGID", "Base", "tag", "fixed");
    ProductSubscription previousProductSubscription("RIGIDOther", "Base", "tag", "fixed");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({newProductSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({previousProductSubscription});
    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnTrueIfProductTagAndRigidChanges)
{
    ProductSubscription newProductSubscription("RIGID", "Base", "tag", "fixed");
    ProductSubscription previousProductSubscription("RIGID", "Base", "tagother", "fixed");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({newProductSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({previousProductSubscription});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

TEST_F(TestConfigurationDataUtils, returnTrueIfProductFixedChanges)
{
    ProductSubscription newProductSubscription("RIGID", "Base", "tag", "fixed");
    ProductSubscription previousProductSubscription("RIGID", "Base", "tag", "fixedother");

    auto newSettings = getValidUpdateSettings();
    newSettings.setProductsSubscription({newProductSubscription});
    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({previousProductSubscription});
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

//checkIfShouldForceInstallAllProducts
TEST_F(TestConfigurationDataUtils, returnsFalseIfNotOnlyCompareSubscriptionsAndFeaturesAndInvalidPreviousSettings)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto newSettings = getValidUpdateSettings();
    auto previousSettings = getValidUpdateSettings();
    ASSERT_FALSE(previousSettings.isVerified());

    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(newSettings, previousSettings, false));
    EXPECT_TRUE(appenderContains("Previous update configuration data has not been set or verified."));
}

TEST_F(TestConfigurationDataUtils, returnsTrueIfForceReinstallAllProductsTrue)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto newSettings = getValidUpdateSettings();
    newSettings.setForceReinstallAllProducts(true);
    auto previousSettings = getValidUpdateSettings();

    ASSERT_TRUE(newSettings.getForceReinstallAllProducts());
    ASSERT_TRUE(newSettings.getProductsSubscription().size() == previousSettings.getProductsSubscription().size());
    ASSERT_TRUE(newSettings.getFeatures().size() == previousSettings.getFeatures().size());

    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(newSettings, previousSettings, true));
    EXPECT_TRUE(appenderContains("Found new subscription or features in update configuration."));
}

TEST_F(TestConfigurationDataUtils, returnsTrueIfFeaturesChange)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto newSettings = getValidUpdateSettings();
    newSettings.setForceReinstallAllProducts(false);
    newSettings.setFeatures({"CORE", "AV"});
    auto previousSettings = getValidUpdateSettings();

    ASSERT_FALSE(newSettings.getForceReinstallAllProducts());
    ASSERT_TRUE(newSettings.getProductsSubscription().size() == previousSettings.getProductsSubscription().size());
    ASSERT_FALSE(newSettings.getFeatures().size() == previousSettings.getFeatures().size());

    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(newSettings, previousSettings, true));
    EXPECT_TRUE(appenderContains("Found new subscription or features in update configuration."));
}

TEST_F(TestConfigurationDataUtils, returnsTrueIfProductSubscriptionListSizeChanges)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto newSettings = getValidUpdateSettings();
    newSettings.setForceReinstallAllProducts(false);
    ProductSubscription productSubscription("RIGID", "Base", "tag", "fixed");
    newSettings.setProductsSubscription({productSubscription});
    auto previousSettings = getValidUpdateSettings();

    ASSERT_FALSE(newSettings.getForceReinstallAllProducts());
    ASSERT_FALSE(newSettings.getProductsSubscription().size() == previousSettings.getProductsSubscription().size());
    ASSERT_TRUE(newSettings.getFeatures().size() == previousSettings.getFeatures().size());

    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(newSettings, previousSettings, true));
    EXPECT_TRUE(appenderContains("Found new subscription or features in update configuration."));
}

TEST_F(TestConfigurationDataUtils, returnsTrueIfProductSubscriptionRigidNameChanges)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto newSettings = getValidUpdateSettings();
    newSettings.setForceReinstallAllProducts(false);
    ProductSubscription newProductSubscription("RIGID", "Base", "tag", "fixed");
    newSettings.setProductsSubscription({newProductSubscription});

    auto previousSettings = getValidUpdateSettings();
    ProductSubscription previousProductSubscription("RIGIDother", "Base", "tag", "fixed");
    previousSettings.setProductsSubscription({previousProductSubscription});

    ASSERT_FALSE(newSettings.getForceReinstallAllProducts());
    ASSERT_TRUE(newSettings.getProductsSubscription().size() == previousSettings.getProductsSubscription().size());
    ASSERT_TRUE(newSettings.getFeatures().size() == previousSettings.getFeatures().size());

    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(newSettings, previousSettings, true));
    EXPECT_TRUE(appenderContains("Subscription list in update configuration has changed."));
}

TEST_F(TestConfigurationDataUtils, returnsTrueIfFeatureListSameSizeButDifferent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto newSettings = getValidUpdateSettings();
    newSettings.setForceReinstallAllProducts(false);
    newSettings.setFeatures({"CORE"});

    auto previousSettings = getValidUpdateSettings();
    previousSettings.setFeatures({"AV"});

    ASSERT_FALSE(newSettings.getForceReinstallAllProducts());
    ASSERT_TRUE(newSettings.getProductsSubscription().size() == previousSettings.getProductsSubscription().size());
    ASSERT_TRUE(newSettings.getFeatures().size() == previousSettings.getFeatures().size());

    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(newSettings, previousSettings, true));
    EXPECT_TRUE(appenderContains("Feature list in update configuration has changed."));
}

TEST_F(TestConfigurationDataUtils, returnsFalseIfSettingsAreTheSame)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto newSettings = getValidUpdateSettings();
    newSettings.setForceReinstallAllProducts(false);
    ProductSubscription productSubscription("RIGID", "Base", "tag", "fixed");
    newSettings.setProductsSubscription({productSubscription});

    auto previousSettings = getValidUpdateSettings();
    previousSettings.setProductsSubscription({productSubscription});

    ASSERT_FALSE(newSettings.getForceReinstallAllProducts());
    ASSERT_TRUE(newSettings.getProductsSubscription().size() == previousSettings.getProductsSubscription().size());
    ASSERT_TRUE(newSettings.getFeatures().size() == previousSettings.getFeatures().size());

    EXPECT_FALSE(ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(newSettings, previousSettings, true));
    EXPECT_TRUE(appenderContains("No difference between new update config and previous update config."));
}