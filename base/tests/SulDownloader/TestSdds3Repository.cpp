// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "MockSdds3Wrapper.h"
#include "MockSusRequester.h"
#include "Sdds3ReplaceAndRestore.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "SulDownloader/sdds3/ISdds3Wrapper.h"
#include "modules/SulDownloader/sdds3/SDDS3Repository.h"
#include "modules/SulDownloader/suldownloaderdata/CatalogueInfo.h"
#include "modules/SulDownloader/suldownloaderdata/DownloadedProduct.h"
#include "sophlib/sdds3/PackageRef.h"
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Common::Policy;
using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class Sdds3RepositoryTest : public MemoryAppenderUsingTests
{
public:
    MockSdds3Wrapper& setupSdds3WrapperAndGetMock()
    {
        auto* sdds3WrapperMock = new StrictMock<MockSdds3Wrapper>();

        auto* pointer = sdds3WrapperMock;
        replacer_.replace(std::unique_ptr<SulDownloader::ISdds3Wrapper>(sdds3WrapperMock));
        return *pointer;
    }
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;

protected:
    Sdds3RepositoryTest() : MemoryAppenderUsingTests("SulDownloaderSDDS3"), usingMemoryAppender_(*this) {}

    Tests::ScopedReplaceSdds3Wrapper replacer_;
    std::unique_ptr<MockFileSystem> mockFileSystem_ = std::make_unique<MockFileSystem>();
    std::unique_ptr<MockSusRequester> mockSusRequester_ = std::make_unique<MockSusRequester>();
    UsingMemoryAppender usingMemoryAppender_;
};

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsSomeProductsHaveChanged)
{
    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        *mockFileSystem_, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["CORE"])"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    SDDS3Repository repository{ std::move(mockSusRequester_) };
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sophlib::sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sophlib::sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "CORE" };
    repository.setFeatures({ "CORE" });

    std::vector<sophlib::sdds3::PackageRef> packagesToInstall = { package1 };
    std::vector<sophlib::sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_, _, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_FALSE(products[1].productHasChanged());
}

TEST_F(
    Sdds3RepositoryTest,
    testGenerateProductListFromSdds3PackageInfoReportsNotInstalledProductsHaveChangedIfFeatureAdded)
{
    // Unit test to address https://sophos.atlassian.net/browse/LINUXDAR-5298
    // The problem is that SUS only tells us what to download, it doesn't know if we've installed it or not.
    // So when we first install only CORE packages, when someone adds a new feature code for something that is
    // contained in the base suite, that package will already be downloaded but not installed. So we need to install
    // packages that SUS tells us to download but also packages that are not installed but in the required
    // features list.
    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        *mockFileSystem_, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["CORE"])"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    SDDS3Repository repository{ std::move(mockSusRequester_) };
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sophlib::sdds3::PackageRef package1;
    package1.lineId_ = "base";
    package1.features_ = { "CORE" };

    sophlib::sdds3::PackageRef package2;
    package2.lineId_ = "another_package";
    package2.features_ = { "CORE" };

    sophlib::sdds3::PackageRef package3;
    package3.lineId_ = "liveterminal";
    package3.features_ = { "LIVETERMINAL" };

    // First update would have happened with just "CORE" as the feature codes
    repository.setFeatures({ "CORE", "LIVETERMINAL" });

    // Get CDN to tell us there are no new packages to install
    std::vector<sophlib::sdds3::PackageRef> packagesToInstall = {};
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));

    // All packages will contain everything, regardless of install state.
    std::vector<sophlib::sdds3::PackageRef> allPackages = { package1, package2, package3 };
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));

    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_, _, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("base");
    auto products = repository.getProducts();

    // This test tries to mimick the 2nd update so base and another_package should not be marked as needing install
    // as they should have already been installed because they have a feature code of CORE.
    EXPECT_EQ(products[0].getLine(), "base");
    EXPECT_FALSE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "another_package");
    EXPECT_FALSE(products[1].productHasChanged());
    // The liveterminal package should be marked as needing install because the LIVETERMINAL feature code is now
    // in the required features list.
    EXPECT_EQ(products[2].getLine(), "liveterminal");
    EXPECT_TRUE(products[2].productHasChanged());
}

TEST_F(Sdds3RepositoryTest, testcheckForMissingPackagesEmptyConfig)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    std::vector<ProductSubscription> subscriptions;
    std::set<std::string> suites;
    repository.checkForMissingPackages(subscriptions, suites);
    EXPECT_EQ(repository.hasError(), false);
}

TEST_F(Sdds3RepositoryTest, testcheckForMissingPackageswithSuceedsWithPackages)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    ProductSubscription sub{ "Plugin1", "", "", "" };
    ProductSubscription sub2{ "Plugin2", "", "", "" };
    std::vector<ProductSubscription> subscriptions{ sub, sub2 };

    std::set<std::string> suites;
    suites.emplace("sdds3.Plugin1");
    suites.emplace("sdds3.Plugin2");
    repository.checkForMissingPackages(subscriptions, suites);
    EXPECT_EQ(repository.hasError(), false);
}

TEST_F(Sdds3RepositoryTest, testcheckForMissingPackagesReportsErrorWithMissingPackages)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    ProductSubscription sub{ "Plugin1", "", "", "" };
    ProductSubscription sub2{ "Plugin2", "", "", "" };
    ProductSubscription sub3{ "Plugin3", "", "", "" };
    std::vector<ProductSubscription> subscriptions{ sub, sub2, sub3 };

    std::set<std::string> suites;
    suites.emplace("sdds3.Plugin1");
    suites.emplace("sdds3.Plugin2");
    repository.checkForMissingPackages(subscriptions, suites);
    EXPECT_EQ(repository.hasError(), true);
}

TEST_F(Sdds3RepositoryTest, checkForMissingPackagesFailsIfSuiteNameIsShorterThanSdds3Prefix)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    std::vector<ProductSubscription> subscriptions{ { "Base", "", "", "" } };

    std::set<std::string> suites{ "sdds3.Base" };
    repository.checkForMissingPackages(subscriptions, suites);
    EXPECT_EQ(repository.hasError(), false);

    std::set<std::string> suites2{ "Base" };
    repository.checkForMissingPackages(subscriptions, suites2);
    EXPECT_EQ(repository.hasError(), true);
}

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsAllProductsHaveChanged)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sophlib::sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sophlib::sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "CORE" };
    repository.setFeatures({ "CORE" });

    std::vector<sophlib::sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sophlib::sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_, _, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_TRUE(products[1].productHasChanged());
}

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsOnlytheProductWithTheRightFeatureCode)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sophlib::sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sophlib::sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "SAV" };
    repository.setFeatures({ "CORE" });

    std::vector<sophlib::sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sophlib::sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getSuites(_, _, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products.size(), 1);
}

TEST_F(
    Sdds3RepositoryTest,
    testGenerateProductListFromSdds3PackageInfoReportsAllProductsThatHaveAtLeastOneFeatureCodeFromTheConfigList)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sophlib::sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sophlib::sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "SAV" };
    repository.setFeatures({ "CORE", "SAV" });

    std::vector<sophlib::sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sophlib::sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getSuites(_, _, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_TRUE(products[1].productHasChanged());
    EXPECT_EQ(products.size(), 2);
}

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoHandlesComponentsWithNoFeatureCode)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sophlib::sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    sophlib::sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    repository.setFeatures({ "CORE" });

    std::vector<sophlib::sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sophlib::sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getSuites(_, _, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products.size(), 0);
}
TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsPrimaryComponentFirst)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sophlib::sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sophlib::sdds3::PackageRef package2;
    package2.lineId_ = "line2_primary";
    package2.features_ = { "CORE" };
    repository.setFeatures({ "CORE" });
    std::vector<sophlib::sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sophlib::sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getSuites(_, _, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);

    // The rigid name passed in is used in a contains check, to ensure that works passing in the first part of the
    // rigid name expected to be at the top of the list when returned.
    repository.generateProductListFromSdds3PackageInfo("line2");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line2_primary");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line1");
    EXPECT_TRUE(products[1].productHasChanged());
}

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsNoProductsHaveChanged)
{
    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        *mockFileSystem_, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["CORE"])"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    SDDS3Repository repository{ std::move(mockSusRequester_) };
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sophlib::sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sophlib::sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "CORE" };
    repository.setFeatures({ "CORE" });

    std::vector<sophlib::sdds3::PackageRef> packagesToInstall = {};
    std::vector<sophlib::sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_, _, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_FALSE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_FALSE(products[1].productHasChanged());
}

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsSupplementOnlyProductsHaveChanged)
{
    EXPECT_CALL(
        *mockFileSystem_,
        exists(Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        *mockFileSystem_, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["CORE"])"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    SDDS3Repository repository{ std::move(mockSusRequester_) };
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    // This is required due to the tryConnect call
    sophlib::sdds3::Config config;
    config.sus_response.suites = { "Suite" };
    EXPECT_CALL(sdds3Wrapper, loadConfig).WillOnce(Return(config));

    sophlib::sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sophlib::sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "CORE" };

    // If package2's supplement has changed, then SDDS3 will report package2 as 'to install'
    // Since CORE is an already installed feature, only package2 should be reported as changed
    std::vector<sophlib::sdds3::PackageRef> packagesToInstall = { package2 };
    std::vector<sophlib::sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_, _, _)).Times(1);

    // The following with set supplement only on repository.
    ConnectionSetup connectionSetup("hello");
    Common::Policy::UpdateSettings configurationData;
    configurationData.setTenantId("hello");
    configurationData.setDeviceId("hello");
    configurationData.setJWToken("hello");
    configurationData.setFeatures({ "CORE" });

    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    std::vector<ProductSubscription> productSubscriptions = { productSubscription };
    configurationData.setProductsSubscription(productSubscriptions);
    repository.tryConnect(connectionSetup, true, configurationData);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_FALSE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_TRUE(products[1].productHasChanged());
}

TEST_F(Sdds3RepositoryTest, tryConnectConnectsToSusIfDoingProductUpdate)
{
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    // SDDS3 config exists and has a suite in it
    EXPECT_CALL(
        *mockFileSystem_,
        exists(Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath()))
        .WillRepeatedly(Return(true));
    sophlib::sdds3::Config config;
    config.sus_response.suites = { "sdds3.Base" };
    EXPECT_CALL(sdds3Wrapper, loadConfig).WillOnce(Return(config));

    SDDS3::SusResponse susResponse{ .data = { .suites = { "sdds3.Base", "sdds3.Suite2" }, .releaseGroups = {} },
                                    .success = true,
                                    .error = "" };
    EXPECT_CALL(*mockSusRequester_, request).WillOnce(Return(susResponse));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    ConnectionSetup connectionSetup("hello");
    bool supplementOnly = false;
    Common::Policy::UpdateSettings configurationData;
    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setProductsSubscription({ productSubscription });

    EXPECT_TRUE(repository.tryConnect(connectionSetup, supplementOnly, configurationData));
    EXPECT_TRUE(appenderContains("DEBUG - Checking suites and release groups with SUS"));
    EXPECT_TRUE(appenderContains("INFO - SUS Request was successful"));
}

TEST_F(Sdds3RepositoryTest, tryConnectConnectsToSusIfDoingSupplementUpdateWithoutCachedSuites)
{
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    // SDDS3 config exists and has no suites
    EXPECT_CALL(
        *mockFileSystem_,
        exists(Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath()))
        .WillRepeatedly(Return(true));
    sophlib::sdds3::Config config;
    EXPECT_CALL(sdds3Wrapper, loadConfig).WillOnce(Return(config));

    SDDS3::SusResponse susResponse{ .data = { .suites = { "sdds3.Base", "sdds3.Suite2" }, .releaseGroups = {} },
                                    .success = true,
                                    .error = "" };
    EXPECT_CALL(*mockSusRequester_, request).WillOnce(Return(susResponse));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    ConnectionSetup connectionSetup("hello");
    bool supplementOnly = true;
    Common::Policy::UpdateSettings configurationData;
    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setProductsSubscription({ productSubscription });

    EXPECT_TRUE(repository.tryConnect(connectionSetup, supplementOnly, configurationData));
    EXPECT_TRUE(appenderContains(
        "ERROR - Supplement-only update requested but no cached SUS response present, doing product update"));
    EXPECT_TRUE(appenderContains("DEBUG - Checking suites and release groups with SUS"));
    EXPECT_TRUE(appenderContains("INFO - SUS Request was successful"));
}

TEST_F(Sdds3RepositoryTest, tryConnectDoesntConnectToSusIfDoingSupplementUpdateWithCachedSuite)
{
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    // SDDS3 config exists and has a suite in it
    EXPECT_CALL(
        *mockFileSystem_,
        exists(Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath()))
        .WillRepeatedly(Return(true));
    sophlib::sdds3::Config config;
    config.sus_response.suites = { "sdds3.Base" };
    EXPECT_CALL(sdds3Wrapper, loadConfig).WillOnce(Return(config));

    EXPECT_CALL(*mockSusRequester_, request).Times(0);

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    ConnectionSetup connectionSetup("hello");
    bool supplementOnly = true;
    Common::Policy::UpdateSettings configurationData;
    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setProductsSubscription({ productSubscription });

    EXPECT_TRUE(repository.tryConnect(connectionSetup, supplementOnly, configurationData));
    EXPECT_TRUE(appenderContains("INFO - Using previously acquired suites and release groups"));
}

TEST_F(Sdds3RepositoryTest, tryConnectFailsIfSusRequestFails)
{
    setupSdds3WrapperAndGetMock();

    SDDS3::SusResponse susResponse{ .data = {}, .success = false, .error = "error message" };
    EXPECT_CALL(*mockSusRequester_, request).WillOnce(Return(susResponse));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    ConnectionSetup connectionSetup("hello");
    bool supplementOnly = false;
    Common::Policy::UpdateSettings configurationData;
    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setProductsSubscription({ productSubscription });

    EXPECT_FALSE(repository.tryConnect(connectionSetup, supplementOnly, configurationData));
    EXPECT_EQ(repository.getError().status, RepositoryStatus::CONNECTIONERROR);
    EXPECT_TRUE(appenderContains("DEBUG - Getting suites failed with: error message"));
}

TEST_F(Sdds3RepositoryTest, tryConnectSetsDOWNLOADFAILEDIfSusRequestFailsWithNonTempraryError)
{
    setupSdds3WrapperAndGetMock();

    SDDS3::SusResponse susResponse{ .data = {}, .success = false, .persistentError = true, .error = "error message" };
    EXPECT_CALL(*mockSusRequester_, request).WillOnce(Return(susResponse));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    ConnectionSetup connectionSetup("hello");
    bool supplementOnly = false;
    Common::Policy::UpdateSettings configurationData;
    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setProductsSubscription({ productSubscription });

    EXPECT_FALSE(repository.tryConnect(connectionSetup, supplementOnly, configurationData));
    EXPECT_EQ(repository.getError().status, RepositoryStatus::DOWNLOADFAILED);
    EXPECT_TRUE(appenderContains("DEBUG - Getting suites failed with: error message"));
}

TEST_F(Sdds3RepositoryTest, tryConnectFailsIfSusRequestReturnsNoSuites)
{
    setupSdds3WrapperAndGetMock();

    SDDS3::SusResponse susResponse{ .data = { .suites = {}, .releaseGroups = { "Release group A" } },
                                    .success = true,
                                    .error = "" };
    EXPECT_CALL(*mockSusRequester_, request).WillOnce(Return(susResponse));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    ConnectionSetup connectionSetup("hello");
    bool supplementOnly = false;
    Common::Policy::UpdateSettings configurationData;
    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setProductsSubscription({ productSubscription });

    EXPECT_FALSE(repository.tryConnect(connectionSetup, supplementOnly, configurationData));
    EXPECT_TRUE(appenderContains("DEBUG - Getting suites failed with: Product doesn't match any suite: Base"));
}

TEST_F(Sdds3RepositoryTest, tryConnectFailsIfSusRequestReturnsNoSuitesAndNoSubscriptionsWereSpecified)
{
    setupSdds3WrapperAndGetMock();

    SDDS3::SusResponse susResponse{ .data = { .suites = {}, .releaseGroups = { "Release group A" } },
                                    .success = true,
                                    .error = "" };
    EXPECT_CALL(*mockSusRequester_, request).WillOnce(Return(susResponse));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    ConnectionSetup connectionSetup("hello");
    bool supplementOnly = false;
    Common::Policy::UpdateSettings configurationData;

    EXPECT_FALSE(repository.tryConnect(connectionSetup, supplementOnly, configurationData));
    EXPECT_TRUE(appenderContains("DEBUG - Getting suites failed with: Product doesn't match any suite: "));
}
