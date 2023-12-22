// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "MockSdds3Wrapper.h"
#include "MockSusRequester.h"
#include "Sdds3ReplaceAndRestore.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Process/IProcess.h"
#include "SulDownloader/SulDownloader.h"
#include "SulDownloader/sdds3/ISdds3Wrapper.h"
#include "SulDownloader/sdds3/SDDS3Repository.h"
#include "SulDownloader/sdds3/Sdds3RepositoryFactory.h"
#include "SulDownloader/sdds3/SusRequester.h"
#include "SulDownloader/suldownloaderdata/DownloadedProduct.h"
#include "SulDownloader/suldownloaderdata/VersigImpl.h"
#include "sophlib/sdds3/PackageRef.h"
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/SystemUtilsReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockHttpRequester.h"
#include "tests/Common/Helpers/MockProcess.h"
#include "tests/Common/Helpers/MockSystemUtils.h"
#include "tests/SulDownloader/JwtUtils.h"
#include "tests/SulDownloader/MockSignatureVerifierWrapper.h"
#include "tests/SulDownloader/MockVersig.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Common::Policy;
using namespace SulDownloader::suldownloaderdata;
using Common::DownloadReport::RepositoryStatus;

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

    void SetUp() override
    {
        mockFileSystem_ = std::make_unique<NiceMock<MockFileSystem>>();
        mockSusRequester_ = std::make_unique<NiceMock<MockSusRequester>>();
    }

    void TearDown() override
    {
        Tests::restoreSystemUtils();
        Tests::restoreFileSystem();
        Tests::restoreSdds3Wrapper();
    }

    Tests::ScopedReplaceSdds3Wrapper replacer_;
    std::unique_ptr<NiceMock<MockFileSystem>> mockFileSystem_;
    std::unique_ptr<NiceMock<MockSusRequester>> mockSusRequester_ ;
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
    EXPECT_CALL(
            *mockFileSystem_,
            isFile(_))
            .WillRepeatedly(Return(true));
    EXPECT_CALL(
            *mockFileSystem_,
            readFile(_))
            .WillRepeatedly(Return(""));
    sophlib::sdds3::Config config;
    config.sus_response.suites = { "sdds3.ServerProtectionLinux-Base" };
    std::string baseSuiteXML = R"(
<suite name="ServerProtectionLinux-Base" version="2023.10.27.1.0" nonce="f5a88fb176" marketing-version="2023.10.27.1 ">
  <package-ref src="SPL-Base-Component_1.2.5.0.1506.f287b5e6e4.zip" size="43199062" sha256="5dcad131df0b33d32b103acef8fc907d9472634c08cc754a4d9633ed8726814f">

    <platforms>
      <platform name="LINUX_ARM64" />
      <platform name="LINUX_INTEL_LIBC6" />
    </platforms>

  </package-ref>
</suite>)";
    EXPECT_CALL(sdds3Wrapper, loadConfig).WillOnce(Return(config));
    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob( _))
            .Times(1)
            .WillOnce(Return(baseSuiteXML));
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

TEST_F(Sdds3RepositoryTest, synchronizeWritesToFileIniWhenDirect)
{
    setupSdds3WrapperAndGetMock();

    const std::string expectedOutputToFile = "usedProxy = false\nusedUpdateCache = false\nusedMessageRelay = false\nproxyOrMessageRelayURL = \n";

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    EXPECT_CALL(*mockSdds3Wrapper, sync).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, getPackagesToInstall(_,_,_,_)).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, getPackages(_,_,_)).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, getSuites(_,_,_)).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, saveConfig(_,_)).Times(1);
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    EXPECT_CALL(*mockFileSystem_, exists(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem_, writeFile(_,expectedOutputToFile)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    auto mockSystemUtils = std::make_unique<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable(_)).WillOnce(Return("imnotempty"));
    Tests::ScopedReplaceSystemUtils scopedSystemUtils (std::move(mockSystemUtils));

    ConnectionSetup connectionSetup("direct");

    Common::Policy::UpdateSettings configurationData;
    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setProductsSubscription({ productSubscription });

    EXPECT_TRUE(repository.synchronize(configurationData, connectionSetup, true));
    EXPECT_EQ(repository.getError().status, RepositoryStatus::SUCCESS);
}

TEST_F(Sdds3RepositoryTest, synchronizeWritesToFileIniWhenUpdateCache)
{
    setupSdds3WrapperAndGetMock();

    const std::string expectedOutputToFile = "usedProxy = false\nusedUpdateCache = true\nusedMessageRelay = false\nproxyOrMessageRelayURL = \n";

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    EXPECT_CALL(*mockSdds3Wrapper, sync).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, getPackagesToInstall(_,_,_,_)).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, getPackages(_,_,_)).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, getSuites(_,_,_)).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, saveConfig(_,_)).Times(1);
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    EXPECT_CALL(*mockFileSystem_, exists(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem_, writeFile(_,expectedOutputToFile)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    SDDS3Repository repository{ std::move(mockSusRequester_) };

    auto mockSystemUtils = std::make_unique<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable(_)).WillOnce(Return("imnotempty"));
    Tests::ScopedReplaceSystemUtils scopedSystemUtils (std::move(mockSystemUtils));

    ConnectionSetup connectionSetup("direct", true);
    Common::Policy::UpdateSettings configurationData;
    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setProductsSubscription({ productSubscription });

    EXPECT_TRUE(repository.synchronize(configurationData, connectionSetup, true));
    EXPECT_EQ(repository.getError().status, RepositoryStatus::SUCCESS);
}

TEST_F(Sdds3RepositoryTest, synchronizeWritesToFileIniWhenProxy)
{
    setupSdds3WrapperAndGetMock();

    const std::string expectedOutputToFile = "usedProxy = true\nusedUpdateCache = false\nusedMessageRelay = false\nproxyOrMessageRelayURL = \n";

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    EXPECT_CALL(*mockSdds3Wrapper, sync).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, getPackagesToInstall(_,_,_,_)).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, getPackages(_,_,_)).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, getSuites(_,_,_)).Times(1);
    EXPECT_CALL(*mockSdds3Wrapper, saveConfig(_,_)).Times(1);
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    EXPECT_CALL(*mockFileSystem_, exists(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem_, writeFile(_,expectedOutputToFile)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem (std::move(mockFileSystem_));

    auto mockSystemUtils = std::make_unique<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable(_)).WillOnce(Return("imnotempty"));
    Tests::ScopedReplaceSystemUtils scopedSystemUtils (std::move(mockSystemUtils));

    SDDS3Repository repository{ std::move(mockSusRequester_) };

    Common::Policy::Proxy proxy("Im a proxy");
    ConnectionSetup connectionSetup("direct", false, proxy);

    Common::Policy::UpdateSettings configurationData;
    ProductSubscription productSubscription("Base", "", "RECOMMENDED", "");
    configurationData.setPrimarySubscription(productSubscription);
    configurationData.setProductsSubscription({ productSubscription });

    EXPECT_TRUE(repository.synchronize(configurationData, connectionSetup, true));
    EXPECT_EQ(repository.getError().status, RepositoryStatus::SUCCESS);
}


class TestSdds3Repository : public LogInitializedTests
{
protected:
    ~TestSdds3Repository() override
    {
        Tests::restoreFileSystem();
        SulDownloader::Sdds3RepositoryFactory::instance().restoreCreator();
        Tests::restoreSdds3Wrapper();
        SulDownloader::suldownloaderdata::VersigFactory::instance().restoreCreator();
        Common::Process::restoreCreator();
    }

    static void replaceSdds3RepositoryCreator(RespositoryCreaterFunc creatorMethod)
    {
        // This cannot be invoked directly in tests because each test is a subclass of this, and they don't inherit
        // being a friend of Sdds3RepositoryFactory
        SulDownloader::Sdds3RepositoryFactory::instance().replaceCreator(std::move(creatorMethod));
    }
};

TEST_F(
    TestSdds3Repository,
    ProductThatDoesntFulfilRequestedFeaturesIsNotDistributedAndDoesntTryToReadVersionFromDistributionDirectory)
{
    auto mockFileSystem = std::make_unique<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, exists).Times(AnyNumber());
    EXPECT_CALL(*mockFileSystem, isFile).Times(AnyNumber());
    EXPECT_CALL(*mockFileSystem, readLines(_)).Times(AnyNumber());
    ON_CALL(*mockFileSystem, isDirectory("output_dir/output")).WillByDefault(Return(false));
    ON_CALL(*mockFileSystem, isDirectory("output_dir")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, readFile("input")).WillByDefault(Return(R"({
 "sophosCdnURLs": [
  "https://sdds3.sophosupd.com",
  "https://sdds3.sophosupd.net"
 ],
 "JWToken": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjkyZmU3N2M2LWIxNjctNGE0Zi04MGI0LWZiZjFkMDcwZjk1YiJ9.eyJkZXZpY2VJZCI6IjlmOGE2M2VlLTc4YjktNDVlZi1hOTNiLTQ1Y2MzZjc5YmUzNiIsInRlbmFudElkIjoiYzY1NTc2Y2ItMDEyYS00YjYzLWFjNTgtOWNkNTk4YTBjMWRmIiwiZmVhdHVyZUNvZGUiOlsiZl9zcnZfbGl2ZXF1ZXJ5IiwiZl9zcnZfbWwiLCJmX3Nydl9zY2hlZHVsZWRxdWVyeSIsImZfc3J2X2F2IiwiZl9zcnZfc3RhYyIsImZfc3J2X21kciIsImZfZW5kcG9pbnRfdXBkYXRpbmciXSwiZGV2aWNlVHlwZSI6InNlcnZlciIsInBsYXRmb3JtIjoicG9zaXgiLCJpYXQiOjE3MDA3NDM1MzAsImV4cCI6MTcwMDgyOTkzMCwiYXVkIjoiaHR0cDovL2Rpcy5jZW50cmFsLnNvcGhvcy5jb20iLCJpc3MiOiJpZHAuZGlzLmNlbnRyYWwuc29waG9zLmNvbSJ9.b-XzUk0Hssj8Tv1RtYeiQVD8pSZePJx9sHzyzIe_YgmGBrRB4Anu2dcABcKYrEAIU_I7ou-kaXQG4QSSZL01w-pFNWZ1ydUP7ZvUMi4SC_p7Ioezcs2dDDQn0C4t7GroRyWYYrGz0W9J3wyRT4Qg5_HCIsH8istvhXomxXQGSAfEuKqsvVfrlG9XRY7HZIQl_FgHypzr7S2dHRNyYSOuO79Ms_S-OqRlZO43Wa9NQ1TNEwzSHXjZRVZhl3N7fxgVHvkPJJfrabVPLFW3N4cmF0y1OUiLdyMIet5sEXvIjpJ_miL0ih8OMhpgSjDEqInfF3dwKexTf7CQaJPPDBF3Sg",
 "tenantId": "c65576cb-012a-4b63-ac58-9cd598a0c1df",
 "deviceId": "9f8a63ee-78b9-45ef-a93b-45cc3f79be36",
 "primarySubscription": {
  "rigidName": "ServerProtectionLinux-Base",
  "baseVersion": "",
  "tag": "RECOMMENDED",
  "fixedVersion": ""
 },
 "features": ["CORE"],
 "sophosSusURL": "https://sus.sophosupd.com",
}
)"));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl/base/update/cache/")).WillByDefault(Return(true));
    // VERSION.ini is not read
    EXPECT_CALL(
        *mockFileSystem,
        exists("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .Times(0);
    EXPECT_CALL(
        *mockFileSystem,
        isFile("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .Times(0);
    EXPECT_CALL(
        *mockFileSystem,
        readLines("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .Times(0);
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockHttpRequester = std::make_shared<MockHTTPRequester>();
    Common::HttpRequests::Response susResponse;
    susResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    susResponse.body = R"({"suites": ["sdds3.ServerProtectionLinux-Base"], "release-groups": []})";
    susResponse.headers["X-Content-Signature"] = generateJwt(susResponse.body);
    susResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    ON_CALL(*mockHttpRequester, post).WillByDefault(Return(susResponse));

    auto mockSignatureVerifierWrapper = std::make_shared<MockSignatureVerifierWrapper>();

    replaceSdds3RepositoryCreator(
        [&mockHttpRequester, &mockSignatureVerifierWrapper]()
        {
            auto susRequester = std::make_unique<SDDS3::SusRequester>(mockHttpRequester, mockSignatureVerifierWrapper);
            return std::make_unique<SDDS3Repository>(std::move(susRequester));
        });

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    sophlib::sdds3::PackageRef packageRef;
    packageRef.version_ = "1.2.3";
    packageRef.lineId_ = "ServerProtectionLinux-Base-component";
    packageRef.name_ = "base";
    // Doesn't have CORE feature
    std::vector<sophlib::sdds3::PackageRef> packageRefs{ packageRef };
    EXPECT_CALL(*mockSdds3Wrapper, getPackages).WillRepeatedly(Return(packageRefs));
    EXPECT_CALL(*mockSdds3Wrapper, extractPackagesTo).Times(0); // Expect no distribution
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    auto mockVersig = std::make_unique<MockVersig>();
    SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([&mockVersig]()
                                                                               { return std::move(mockVersig); });

    Common::Process::replaceCreator([]() { return std::make_unique<MockProcess>(); });

    SulDownloader::fileEntriesAndRunDownloader("input", "output_dir/output", "supplement_only.marker", 0s);
}

TEST_F(TestSdds3Repository, SyncErrorDoesNotDistributedAndDoesntTryToReadVersionFromDistributionDirectory)
{
    auto mockFileSystem = std::make_unique<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, exists).Times(AnyNumber());
    EXPECT_CALL(*mockFileSystem, isFile).Times(AnyNumber());
    EXPECT_CALL(*mockFileSystem, readLines(_)).Times(AnyNumber());
    ON_CALL(*mockFileSystem, isDirectory("output_dir/output")).WillByDefault(Return(false));
    ON_CALL(*mockFileSystem, isDirectory("output_dir")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, readFile("input")).WillByDefault(Return(R"({
 "sophosCdnURLs": [
  "https://sdds3.sophosupd.com",
  "https://sdds3.sophosupd.net"
 ],
 "JWToken": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjkyZmU3N2M2LWIxNjctNGE0Zi04MGI0LWZiZjFkMDcwZjk1YiJ9.eyJkZXZpY2VJZCI6IjlmOGE2M2VlLTc4YjktNDVlZi1hOTNiLTQ1Y2MzZjc5YmUzNiIsInRlbmFudElkIjoiYzY1NTc2Y2ItMDEyYS00YjYzLWFjNTgtOWNkNTk4YTBjMWRmIiwiZmVhdHVyZUNvZGUiOlsiZl9zcnZfbGl2ZXF1ZXJ5IiwiZl9zcnZfbWwiLCJmX3Nydl9zY2hlZHVsZWRxdWVyeSIsImZfc3J2X2F2IiwiZl9zcnZfc3RhYyIsImZfc3J2X21kciIsImZfZW5kcG9pbnRfdXBkYXRpbmciXSwiZGV2aWNlVHlwZSI6InNlcnZlciIsInBsYXRmb3JtIjoicG9zaXgiLCJpYXQiOjE3MDA3NDM1MzAsImV4cCI6MTcwMDgyOTkzMCwiYXVkIjoiaHR0cDovL2Rpcy5jZW50cmFsLnNvcGhvcy5jb20iLCJpc3MiOiJpZHAuZGlzLmNlbnRyYWwuc29waG9zLmNvbSJ9.b-XzUk0Hssj8Tv1RtYeiQVD8pSZePJx9sHzyzIe_YgmGBrRB4Anu2dcABcKYrEAIU_I7ou-kaXQG4QSSZL01w-pFNWZ1ydUP7ZvUMi4SC_p7Ioezcs2dDDQn0C4t7GroRyWYYrGz0W9J3wyRT4Qg5_HCIsH8istvhXomxXQGSAfEuKqsvVfrlG9XRY7HZIQl_FgHypzr7S2dHRNyYSOuO79Ms_S-OqRlZO43Wa9NQ1TNEwzSHXjZRVZhl3N7fxgVHvkPJJfrabVPLFW3N4cmF0y1OUiLdyMIet5sEXvIjpJ_miL0ih8OMhpgSjDEqInfF3dwKexTf7CQaJPPDBF3Sg",
 "tenantId": "c65576cb-012a-4b63-ac58-9cd598a0c1df",
 "deviceId": "9f8a63ee-78b9-45ef-a93b-45cc3f79be36",
 "primarySubscription": {
  "rigidName": "ServerProtectionLinux-Base",
  "baseVersion": "",
  "tag": "RECOMMENDED",
  "fixedVersion": ""
 },
 "features": ["CORE"],
 "sophosSusURL": "https://sus.sophosupd.com",
}
)"));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl/base/update/cache/")).WillByDefault(Return(true));
    // VERSION.ini is not read
    EXPECT_CALL(
        *mockFileSystem,
        exists("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .Times(0);
    EXPECT_CALL(
        *mockFileSystem,
        isFile("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .Times(0);
    EXPECT_CALL(
        *mockFileSystem,
        readLines("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .Times(0);
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockHttpRequester = std::make_shared<MockHTTPRequester>();
    Common::HttpRequests::Response susResponse;
    susResponse.errorCode = Common::HttpRequests::ResponseErrorCode::FAILED;
    EXPECT_CALL(*mockHttpRequester, post)
        .WillRepeatedly(Return(susResponse)); // Makes sure that the code gets to the point of trying to synchronize

    auto mockSignatureVerifierWrapper = std::make_shared<MockSignatureVerifierWrapper>();

    replaceSdds3RepositoryCreator(
        [&mockHttpRequester, &mockSignatureVerifierWrapper]()
        {
            auto susRequester = std::make_unique<SDDS3::SusRequester>(mockHttpRequester, mockSignatureVerifierWrapper);
            return std::make_unique<SDDS3Repository>(std::move(susRequester));
        });

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    EXPECT_CALL(*mockSdds3Wrapper, extractPackagesTo).Times(0); // Expect no distribution
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    auto mockVersig = std::make_unique<MockVersig>();
    SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([&mockVersig]()
                                                                               { return std::move(mockVersig); });

    Common::Process::replaceCreator([]() { return std::make_unique<MockProcess>(); });

    SulDownloader::fileEntriesAndRunDownloader("input", "output_dir/output", "supplement_only.marker", 0s);
}

TEST_F(TestSdds3Repository, DistributionFailsSoVersionIniIsNotRead)
{
    auto mockFileSystem = std::make_unique<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, exists).Times(AnyNumber());
    EXPECT_CALL(*mockFileSystem, isFile).Times(AnyNumber());
    EXPECT_CALL(*mockFileSystem, readLines(_)).Times(AnyNumber());
    ON_CALL(*mockFileSystem, isDirectory("output_dir/output")).WillByDefault(Return(false));
    ON_CALL(*mockFileSystem, isDirectory("output_dir")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, readFile("input")).WillByDefault(Return(R"({
 "sophosCdnURLs": [
  "https://sdds3.sophosupd.com",
  "https://sdds3.sophosupd.net"
 ],
 "JWToken": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjkyZmU3N2M2LWIxNjctNGE0Zi04MGI0LWZiZjFkMDcwZjk1YiJ9.eyJkZXZpY2VJZCI6IjlmOGE2M2VlLTc4YjktNDVlZi1hOTNiLTQ1Y2MzZjc5YmUzNiIsInRlbmFudElkIjoiYzY1NTc2Y2ItMDEyYS00YjYzLWFjNTgtOWNkNTk4YTBjMWRmIiwiZmVhdHVyZUNvZGUiOlsiZl9zcnZfbGl2ZXF1ZXJ5IiwiZl9zcnZfbWwiLCJmX3Nydl9zY2hlZHVsZWRxdWVyeSIsImZfc3J2X2F2IiwiZl9zcnZfc3RhYyIsImZfc3J2X21kciIsImZfZW5kcG9pbnRfdXBkYXRpbmciXSwiZGV2aWNlVHlwZSI6InNlcnZlciIsInBsYXRmb3JtIjoicG9zaXgiLCJpYXQiOjE3MDA3NDM1MzAsImV4cCI6MTcwMDgyOTkzMCwiYXVkIjoiaHR0cDovL2Rpcy5jZW50cmFsLnNvcGhvcy5jb20iLCJpc3MiOiJpZHAuZGlzLmNlbnRyYWwuc29waG9zLmNvbSJ9.b-XzUk0Hssj8Tv1RtYeiQVD8pSZePJx9sHzyzIe_YgmGBrRB4Anu2dcABcKYrEAIU_I7ou-kaXQG4QSSZL01w-pFNWZ1ydUP7ZvUMi4SC_p7Ioezcs2dDDQn0C4t7GroRyWYYrGz0W9J3wyRT4Qg5_HCIsH8istvhXomxXQGSAfEuKqsvVfrlG9XRY7HZIQl_FgHypzr7S2dHRNyYSOuO79Ms_S-OqRlZO43Wa9NQ1TNEwzSHXjZRVZhl3N7fxgVHvkPJJfrabVPLFW3N4cmF0y1OUiLdyMIet5sEXvIjpJ_miL0ih8OMhpgSjDEqInfF3dwKexTf7CQaJPPDBF3Sg",
 "tenantId": "c65576cb-012a-4b63-ac58-9cd598a0c1df",
 "deviceId": "9f8a63ee-78b9-45ef-a93b-45cc3f79be36",
 "primarySubscription": {
  "rigidName": "ServerProtectionLinux-Base",
  "baseVersion": "",
  "tag": "RECOMMENDED",
  "fixedVersion": ""
 },
 "features": ["CORE"],
 "sophosSusURL": "https://sus.sophosupd.com",
}
)"));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl/base/update/cache/")).WillByDefault(Return(true));
    // VERSION.ini is not read
    EXPECT_CALL(
        *mockFileSystem,
        exists("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .Times(0);
    EXPECT_CALL(
        *mockFileSystem,
        isFile("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .Times(0);
    EXPECT_CALL(
        *mockFileSystem,
        readLines("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .Times(0);
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockHttpRequester = std::make_shared<MockHTTPRequester>();
    Common::HttpRequests::Response susResponse;
    susResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    susResponse.body = R"({"suites": ["sdds3.ServerProtectionLinux-Base"], "release-groups": []})";
    susResponse.headers["X-Content-Signature"] = generateJwt(susResponse.body);
    susResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    ON_CALL(*mockHttpRequester, post).WillByDefault(Return(susResponse));

    auto mockSignatureVerifierWrapper = std::make_shared<MockSignatureVerifierWrapper>();

    replaceSdds3RepositoryCreator(
        [&mockHttpRequester, &mockSignatureVerifierWrapper]()
        {
            auto susRequester = std::make_unique<SDDS3::SusRequester>(mockHttpRequester, mockSignatureVerifierWrapper);
            return std::make_unique<SDDS3Repository>(std::move(susRequester));
        });

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    sophlib::sdds3::PackageRef packageRef;
    packageRef.version_ = "1.2.4";
    packageRef.features_.emplace_back("CORE");
    packageRef.lineId_ = "ServerProtectionLinux-Base-component";
    packageRef.name_ = "base";
    std::vector<sophlib::sdds3::PackageRef> packageRefs{ packageRef };
    EXPECT_CALL(*mockSdds3Wrapper, getPackages).WillRepeatedly(Return(packageRefs));
    EXPECT_CALL(*mockSdds3Wrapper, extractPackagesTo).WillRepeatedly(Throw(std::runtime_error{ "error" }));
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    auto mockVersig = std::make_unique<MockVersig>();
    SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([&mockVersig]()
                                                                               { return std::move(mockVersig); });

    Common::Process::replaceCreator([]() { return std::make_unique<MockProcess>(); });

    SulDownloader::fileEntriesAndRunDownloader("input", "output_dir/output", "supplement_only.marker", 0s);
}
