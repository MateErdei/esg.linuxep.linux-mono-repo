// Copyright 2023 Sophos All rights reserved.

#include "Common/Policy/UpdateSettings.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"

using namespace Common::Policy;

namespace
{
    class TestUpdateSettings : public MemoryAppenderUsingTests
    {
    public:
        TestUpdateSettings() : MemoryAppenderUsingTests("Policy")
        {}

        static UpdateSettings getValidUpdateSettings()
        {
            UpdateSettings validSettings;
            validSettings.setSophosLocationURLs({"http://really_sophos.info"});
            validSettings.setCredentials({"username", "password"});
            validSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
            validSettings.setFeatures({"CORE"});
            return validSettings;
        }

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

TEST_F(
    TestUpdateSettings,
    settingsAreValidForV2)
{
    setupFileSystemAndGetMock();
    UpdateSettings validSettings = getValidUpdateSettings();
    bool verify = validSettings.verifySettingsAreValid();
    if (!verify)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 200 });
        ASSERT_TRUE(verify);
    }

    std::vector<UpdateSettings> all_invalid_cases;

    auto noPrimarySubscriptionConfig(validSettings);
    noPrimarySubscriptionConfig.setPrimarySubscription({});
    all_invalid_cases.emplace_back(noPrimarySubscriptionConfig);

    auto primaryWithRigidNameOnly(validSettings);
    primaryWithRigidNameOnly.setPrimarySubscription({ "rigidname", "", "", "" });
    all_invalid_cases.emplace_back(primaryWithRigidNameOnly);

    auto primaryWithOutRigidName(validSettings);
    primaryWithOutRigidName.setPrimarySubscription({ "", "baseversion", "RECOMMENDED", "None" });
    all_invalid_cases.emplace_back(primaryWithOutRigidName);

    auto primaryWithRigidNameAndBaseVersion(validSettings);
    primaryWithRigidNameAndBaseVersion.setPrimarySubscription({ "rigidname", "baseversion", "", "" });
    all_invalid_cases.emplace_back(primaryWithRigidNameAndBaseVersion);

    auto productsWithRigidNameOnly(validSettings);
    productsWithRigidNameOnly.setProductsSubscription(
        { ProductSubscription("rigidname", "", "RECOMMENDED", ""), ProductSubscription("rigidname", "", "", "") });
    all_invalid_cases.emplace_back(productsWithRigidNameOnly);

    auto noCoreFeature(validSettings);
    noCoreFeature.setFeatures({ "SAV", "MDR", "SENSOR" });
    all_invalid_cases.emplace_back(noCoreFeature);

    auto noFeatureSet(validSettings);
    noFeatureSet.setFeatures({});
    all_invalid_cases.emplace_back(noFeatureSet);

    for (auto& configData : all_invalid_cases)
    {
        EXPECT_FALSE(configData.verifySettingsAreValid());
    }

    std::vector<UpdateSettings> all_valid_cases;
    auto primaryWithTag(validSettings);
    primaryWithTag.setPrimarySubscription({ "rigidname", "baseversion", "RECOMMENDED", "" });
    all_valid_cases.emplace_back(primaryWithTag);

    auto primaryWithoutBaseVersion(validSettings);
    primaryWithoutBaseVersion.setPrimarySubscription({ "rigidname", "", "RECOMMENDED", "" });
    all_valid_cases.emplace_back(primaryWithoutBaseVersion);

    auto primaryWithTagAndFixedVersion(validSettings);
    primaryWithTagAndFixedVersion.setPrimarySubscription({ "rigidname", "", "RECOMMENDED", "9.1" });
    all_valid_cases.emplace_back(primaryWithTagAndFixedVersion);

    auto primaryWithOnlyFixedVersion(validSettings);
    primaryWithOnlyFixedVersion.setPrimarySubscription({ "rigidname", "", "", "9.1" });
    all_valid_cases.emplace_back(primaryWithOnlyFixedVersion);

    auto featuresContainCORE(validSettings);
    featuresContainCORE.setFeatures({ { "CORE" }, { "MDR" } });
    all_valid_cases.emplace_back(featuresContainCORE);

    auto onlyCOREinFeatures(validSettings);
    onlyCOREinFeatures.setFeatures({ { "CORE" } });
    all_valid_cases.emplace_back(onlyCOREinFeatures);

    auto moreThanOneProduct(validSettings);
    moreThanOneProduct.setProductsSubscription({ { "p1", "", "RECOMMENDED", "" }, { "p2", "", "", "9.1" } });
    all_valid_cases.emplace_back(moreThanOneProduct);

    auto onlyPrimaryAvailable(validSettings);
    onlyPrimaryAvailable.setProductsSubscription({});
    all_valid_cases.emplace_back(onlyPrimaryAvailable);

    for (auto& configData : all_valid_cases)
    {
        EXPECT_TRUE(configData.verifySettingsAreValid());
    }
}

