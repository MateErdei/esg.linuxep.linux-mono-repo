/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "MockSdds3Wrapper.h"
#include "Sdds3ReplaceAndRestore.h"
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/SulDownloader/suldownloaderdata/CatalogueInfo.h>
#include <modules/SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <modules/SulDownloader/sdds3/SDDS3Repository.h>
#include <SulDownloader/sdds3/ISdds3Wrapper.h>

#include <PackageRef.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class Sdds3RepositoryTest : public ::testing::Test
{
public:
    void SetUp() override
    {

        Test::SetUp();
    }
    /**
    * Remove the temporary directory.
    */
    void TearDown() override
    {
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

//    getPackagesIncludingSupplements(*m_session.get(), m_repo, m_config);
//    m_sdds3Wrapper->getPackagesToInstall(*m_session.get(), m_repo, m_config, m_oldConfig);
//    m_sdds3Wrapper->getPackages(*m_session.get(), m_repo, m_config);
    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";

    std::vector<sdds3::PackageRef> packagesToInstall = {package1};
    std::vector<sdds3::PackageRef> allPackages = {package1, package2};
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_,_,_,_)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_,_,_)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_,_)).Times(1);

    repository.generateProductListFromSdds3PackageInfo();
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_FALSE(products[1].productHasChanged());
}

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsAllProductsHaveChanged) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";

    std::vector<sdds3::PackageRef> packagesToInstall = { package1, package2 };
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo();
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_TRUE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_TRUE(products[1].productHasChanged());
}

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsNoProductsHaveChanged) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";

    std::vector<sdds3::PackageRef> packagesToInstall = { };
    std::vector<sdds3::PackageRef> allPackages = { package1, package2 };
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_, _, _, _)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_, _, _)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_, _)).Times(1);

    repository.generateProductListFromSdds3PackageInfo();
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_FALSE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_FALSE(products[1].productHasChanged());
}

TEST_F(Sdds3RepositoryTest, testGenerateProductListFromSdds3PackageInfoReportsSupplementOnlyProductsHaveChanged) // NOLINT
{
    SDDS3Repository repository;
    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();

    sdds3::PackageRef package1;
    package1.lineId_ = "line1";
    sdds3::PackageRef package2;
    package2.lineId_ = "line2";

    std::vector<sdds3::PackageRef> packagesToInstall = {package1, package2};
    std::vector<sdds3::PackageRef> allPackages = {package1, package2};
    std::vector<sdds3::PackageRef> packagesWithSupplements = {package2};
    EXPECT_CALL(sdds3Wrapper, getPackagesToInstall(_,_,_,_)).WillOnce(Return(packagesToInstall));
    EXPECT_CALL(sdds3Wrapper, getPackages(_,_,_)).WillOnce(Return(allPackages));
    EXPECT_CALL(sdds3Wrapper, getPackagesIncludingSupplements(_,_,_)).WillOnce(Return(packagesWithSupplements));
    EXPECT_CALL(sdds3Wrapper, saveConfig(_,_)).Times(1);

    // The following with set supplement only on repository.
    ConnectionSetup connectionSetup("hello");
    ConfigurationData configurationData;
    configurationData.setTenantId("hello");
    configurationData.setDeviceId("hello");
    configurationData.setJWToken("hello");

    ProductSubscription productSubscription("Base","","RECOMMENDED", "");
    std::vector<ProductSubscription> productSubscriptions = {productSubscription};
    configurationData.setProductsSubscription(productSubscriptions);
    repository.tryConnect(connectionSetup,true, configurationData);

    repository.generateProductListFromSdds3PackageInfo();
    auto products = repository.getProducts();

    EXPECT_EQ(products[0].getLine(), "line1");
    EXPECT_FALSE(products[0].productHasChanged());
    EXPECT_EQ(products[1].getLine(), "line2");
    EXPECT_TRUE(products[1].productHasChanged());
}

