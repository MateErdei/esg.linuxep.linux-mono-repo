/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockSdds3Wrapper.h"
#include "Sdds3ReplaceAndRestore.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <SulDownloader/sdds3/ISdds3Wrapper.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/SulDownloader/sdds3/SDDS3Repository.h>
#include <modules/SulDownloader/suldownloaderdata/CatalogueInfo.h>
#include <modules/SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>

#include <PackageRef.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class Sdds3RepositoryTest : public ::testing::Test
{
public:
    MockedApplicationPathManager* m_mockAppManager = new NiceMock<MockedApplicationPathManager>();
    void SetUp() override
    {
        Test::SetUp();
        MockedApplicationPathManager& mock(*m_mockAppManager);
        ON_CALL(mock, getUpdateCertificatesPath()).WillByDefault(Return("/etc/ssl/cert"));

        Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(m_mockAppManager));
    }
    /**
     * Remove the temporary directory.
     */
    void TearDown() override
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        Tests::restoreSdds3Wrapper();
        Test::TearDown();
    }
    MockSdds3Wrapper& setupSdds3WrapperAndGetMock(int expectCallCount = 1)
    {
        auto* sdds3WrapperMock = new StrictMock<MockSdds3Wrapper>();

        auto* pointer = sdds3WrapperMock;
        m_replacer.replace(std::unique_ptr<SulDownloader::ISdds3Wrapper>(sdds3WrapperMock));
        return *pointer;
    }
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;

protected:
    Tests::ScopedReplaceSdds3Wrapper m_replacer;
};

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsSomeProductsHaveChanged) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    ON_CALL(*m_mockAppManager, getFeaturesJsonPath()).WillByDefault(Return("InstalledFeatures.json"));
    EXPECT_CALL(
        *filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        *filesystemMock, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["CORE"])"));

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "CORE" };
    repository.setFeatures({ "CORE" });

    std::vector<sdds3::PackageRef> packagesToInstall = { package1 };
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_,_,_)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_FALSE(products[1].productHasChanged());
}

TEST_F(
    Sdds3RepositoryTest,
    testGenerateProductListFromSdds3PackageInfoReportsNotInstalledProductsHaveChangedIfFeatureAdded) // NOLINT
{
    // Unit test to address https://sophos.atlassian.net/browse/LINUXDAR-5298
    // The problem is that SUS only tells us what to download, it doesn't know if we've installed it or not.
    // So when we first install only CORE packages, when someone adds a new feature code for something that is
    // contained in the base suite, that package will already be downloaded but not installed. So we need to install
    // packages that SUS tells us to download but also packages that are not installed but in the required
    // features list.
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    ON_CALL(*m_mockAppManager, getFeaturesJsonPath()).WillByDefault(Return("InstalledFeatures.json"));
    EXPECT_CALL(
        *filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        *filesystemMock, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["CORE"])"));

    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "base";
    package1.features_ = { "CORE" };

    sdds3::PackageRef package2;
    package2.lineId_ = "another_package";
    package2.features_ = { "CORE" };

    sdds3::PackageRef package3;
    package3.lineId_ = "liveterminal";
    package3.features_ = { "LIVETERMINAL" };

    // First update would have happened with just "CORE" as the feature codes
    repository.setFeatures({ "CORE", "LIVETERMINAL" });

    // Get SUS to tell us there are no new packages to install
    std::vector<sdds3::PackageRef> packagesToInstall = {};
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));

    // All packages will contain everything, regardless of install state.
    std::vector<sdds3::PackageRef> allPackages = { package1, package2, package3 };
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));

    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_,_,_)).Times(1);

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

TEST_F(Sdds3RepositoryTest, testcheckForMissingPackagesEmptyConfig) // NOLINT
{
    SDDS3Repository repository;

    std::vector<ProductSubscription> subscriptions;
    std::set<std::string> suites;
    repository.checkForMissingPackages(subscriptions, suites);
    EXPECT_EQ(repository.hasError(), false);
}

TEST_F(Sdds3RepositoryTest, testcheckForMissingPackageswithSuceedsWithPackages) // NOLINT
{
    SDDS3Repository repository;

    ProductSubscription sub{ "Plugin1", "", "", "" };
    ProductSubscription sub2{ "Plugin2", "", "", "" };
    std::vector<ProductSubscription> subscriptions{ sub, sub2 };

    std::set<std::string> suites;
    suites.emplace("sdds3.Plugin1");
    suites.emplace("sdds3.Plugin2");
    repository.checkForMissingPackages(subscriptions, suites);
    EXPECT_EQ(repository.hasError(), false);
}

TEST_F(Sdds3RepositoryTest, testcheckForMissingPackagesReportsErrorWithMissingPackages) // NOLINT
{
    SDDS3Repository repository;

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

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsAllProductsHaveChanged) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "CORE" };
    repository.setFeatures({ "CORE" });

    std::vector<sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_,_,_)).Times(1);


    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_TRUE(products[1].productHasChanged());
}

TEST_F(
    Sdds3RepositoryTest,
    testGenerateProductListFromSdds3PackageInfoReportsOnlytheProductWithTheRightFeatureCode) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "SAV" };
    repository.setFeatures({ "CORE" });

    std::vector<sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getSuites(_,_,_)).Times(1);
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products.size(), 1);
}

TEST_F(
    Sdds3RepositoryTest,
    testGenerateProductListFromSdds3PackageInfoReportsAllProductsThatHaveAtLeastOneFeatureCodeFromTheConfigList) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "SAV" };
    repository.setFeatures({ "CORE", "SAV" });

    std::vector<sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getSuites(_,_,_)).Times(1);
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_TRUE(products[1].productHasChanged());
    EXPECT_EQ(products.size(), 2);
}

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoHandlesComponentsWithNoFeatureCode) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    repository.setFeatures({ "CORE" });

    std::vector<sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getSuites(_,_,_)).Times(1);
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products.size(), 0);
}
TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsPrimaryComponentFirst) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sdds3::PackageRef package2;
    package2.lineId_ = "line2_primary";
    package2.features_ = { "CORE" };
    repository.setFeatures({ "CORE" });
    std::vector<sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getSuites(_,_,_)).Times(1);
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

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsNoProductsHaveChanged) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    ON_CALL(*m_mockAppManager, getFeaturesJsonPath()).WillByDefault(Return("InstalledFeatures.json"));
    EXPECT_CALL(
        *filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        *filesystemMock, readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["CORE"])"));

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "CORE" };
    repository.setFeatures({ "CORE" });

    std::vector<sdds3::PackageRef> packagesToInstall = {};
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_,_,_)).Times(1);


    repository.generateProductListFromSdds3PackageInfo("line1");
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_FALSE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_FALSE(products[1].productHasChanged());
}

TEST_F(
    Sdds3RepositoryTest,
    testGenerateProductListFromSdds3PackageInfoReportsSupplementOnlyProductsHaveChanged) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    package1.features_ = { "CORE" };
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";
    package2.features_ = { "CORE" };

    std::vector<sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    std::vector<sdds3::PackageRef> packagesWithSupplements = { package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getPackagesIncludingSupplements(_, _, _)).WillOnce(Return(packagesWithSupplements));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);
    EXPECT_CALL(sdds3Wrapper, getSuites(_,_,_)).Times(1);

    // The following with set supplement only on repository.
    ConnectionSetup connectionSetup("hello");
    ConfigurationData configurationData;
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
