// Copyright 2023 Sophos All rights reserved.

#include "Common/Policy/UpdateSettings.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include "TestUpdateSettingsBase.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

using namespace Common::Policy;

namespace
{
    class TestUpdateSettings : public TestUpdateSettingsBase
    {
    public:
        MockFileSystem& setupFileSystemAndGetMock()
        {
            using ::testing::Ne;
            Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, "/installroot");

            auto filesystemMock = std::make_unique<NiceMock<MockFileSystem>>();
            ON_CALL(*filesystemMock, isDirectory(Common::ApplicationConfiguration::applicationPathManager().sophosInstall())).WillByDefault(Return(true));
            ON_CALL(*filesystemMock, isDirectory(Common::ApplicationConfiguration::applicationPathManager().getLocalWarehouseStoreDir())).WillByDefault(Return(true));

            std::string empty;
            ON_CALL(*filesystemMock, exists(empty)).WillByDefault(Return(false));
            ON_CALL(*filesystemMock, exists(Ne(empty))).WillByDefault(Return(true));

            auto* borrowedPtr = filesystemMock.get();
            replacer_.replace(std::move(filesystemMock));
            return *borrowedPtr;
        }
        Tests::ScopedReplaceFileSystem replacer_;
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
}

TEST_F(TestUpdateSettings, validSettingsAreValid)
{
    setupFileSystemAndGetMock();
    UpdateSettings validSettings = getValidUpdateSettings();
    EXPECT_TRUE(validSettings.verifySettingsAreValid());
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