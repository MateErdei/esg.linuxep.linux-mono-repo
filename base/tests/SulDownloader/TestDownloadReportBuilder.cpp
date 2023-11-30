// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "MockSdds3Repository.h"
#include "TestSdds3RepositoryHelper.h"

#include "Common/DownloadReport/DownloadReport.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "SulDownloader/suldownloaderdata/DownloadReportBuilder.h"
#include "SulDownloader/suldownloaderdata/DownloadedProduct.h"
#include "SulDownloader/suldownloaderdata/TimeTracker.h"
#include "tests/Common/Helpers/LogInitializedTests.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Common::UtilityImpl;

using namespace SulDownloader::suldownloaderdata;
using namespace Common::DownloadReport;

class DownloadReportTest : public LogOffInitializedTests
{
public:
    void SetUp() override {}

    void TearDown() override {}

    static SulDownloader::suldownloaderdata::ProductMetadata createTestProductMetaData()
    {
        SulDownloader::suldownloaderdata::ProductMetadata metadata;

        metadata.setLine("ProductLine1");
        metadata.setDefaultHomePath("Linux");
        metadata.setVersion("1.1.1.1");
        metadata.setName("Linux Security");
        Tag tag("RECOMMENDED", "1", "RECOMMENDED");
        metadata.setTags({ tag });

        return metadata;
    }

    static SulDownloader::suldownloaderdata::DownloadedProduct createTestDownloadedProduct(
        SulDownloader::suldownloaderdata::ProductMetadata& metadata)
    {
        SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct(metadata);

        // set default data for tests
        downloadedProduct.setProductHasChanged(false);
        downloadedProduct.setDistributePath("/tmp");

        return downloadedProduct;
    }

    static SulDownloader::suldownloaderdata::ProductMetadata createTestUninstalledProductMetaData()
    {
        SulDownloader::suldownloaderdata::ProductMetadata metadata;

        metadata.setLine("UninstalledProductLine1");

        return metadata;
    }

    static SulDownloader::suldownloaderdata::DownloadedProduct createTestUninstalledProduct(
        SulDownloader::suldownloaderdata::ProductMetadata& metadata)
    {
        SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct(metadata);

        // set default data for tests
        downloadedProduct.setProductIsBeingUninstalled(true);

        return downloadedProduct;
    }

    static TimeTracker createTimeTracker()
    {
        TimeTracker timeTracker;

        time_t currentTime;
        currentTime = TimeUtils::getCurrTime();

        timeTracker.setStartTime(currentTime - 30);
        timeTracker.setFinishedTime(currentTime - 1);
        return timeTracker;
    }

    static void checkReportValue(DownloadReport& report, SulDownloader::suldownloaderdata::ProductMetadata& metadata)
    {
        EXPECT_EQ(report.getProducts().size(), 1);

        EXPECT_STREQ(report.getProducts()[0].rigidName.c_str(), metadata.getLine().c_str());
        EXPECT_STREQ(report.getProducts()[0].name.c_str(), metadata.getName().c_str());
        EXPECT_STREQ(report.getProducts()[0].downloadedVersion.c_str(), metadata.getVersion().c_str());
    }

    static void checkJsonOutput(const DownloadReport& report, const std::string& jsonString)
    {
        // example jsonString being passed
        //"{\n \"startTime\": \"20180621 142342\",\n \"finishTime\": \"20180621 142411\",\n \"syncTime\": \"20180621
        // 142352\",\n \"status\": \"SUCCESS\",\n \"sulError\": \"\",\n \"errorDescription\": \"\",\n \"products\": [\n
        //{\n   \"rigidName\": \"ProductLine1\",\n   \"productName\": \"Linux Security\",\n   \"installedVersion\":
        //\"1.1.1.1\",\n   \"downloadVersion\": \"1.1.1.1\",\n   \"errorDescription\": \"\"\n  }\n ]\n}\n"

        std::string valueToFind;

        valueToFind = "startTime\": \"" + report.getStartTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "finishTime\": \"" + report.getFinishedTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "status\": \"" + Common::DownloadReport::toString(report.getStatus());
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "sulError\": \"" + report.getSulError();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "errorDescription\": \"" + report.getDescription();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "urlSource\": \"" + report.getSourceURL();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        // double check to ensure we have something to loop through.
        EXPECT_NE(report.getProducts().size(), 0);

        for (auto& product : report.getProducts())
        {
            valueToFind = "rigidName\": \"" + product.rigidName;
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

            valueToFind = "productName\": \"" + product.name;
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

            valueToFind = "downloadVersion\": \"" + product.downloadedVersion;
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

            valueToFind = "errorDescription\": \"" + product.errorDescription;
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

            valueToFind = R"("productStatus": ")" + product.jsonString() + "\"";
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));
        }

        valueToFind = "supplementOnlyUpdate\": " + std::string{ report.isSupplementOnlyUpdate() ? "true" : "false" };
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));
    }

    static void checkJsonOutputWithNoProducts(DownloadReport& report, std::string& jsonString)
    {
        // example jsonString being passed
        //"{\n \"startTime\": \"20180621 142342\",\n \"finishTime\": \"20180621 142411\",\n \"syncTime\": \"20180621
        // 142352\",\n \"status\": \"SUCCESS\",\n \"sulError\": \"\",\n \"errorDescription\": \"\",\n \"products\": [\n
        // {
        //}\n ]\n}\n"

        std::string valueToFind;

        valueToFind = "startTime\": \"" + report.getStartTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "finishTime\": \"" + report.getFinishedTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "status\": \"" + Common::DownloadReport::toString(report.getStatus());
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "sulError\": \"" + report.getSulError();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "errorDescription\": \"" + report.getDescription();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        // double check to ensure we have something to loop through.
        EXPECT_EQ(report.getProducts().size(), 0);
    }

    static std::
        pair<std::vector<suldownloaderdata::DownloadedProduct>, std::vector<suldownloaderdata::SubscriptionInfo>>
        componentSuiteSubscriptionReferencingTwoProducts()
    {
        suldownloaderdata::RepositoryError wErr;
        wErr.Description = "Error P1";
        wErr.status = RepositoryStatus::UNINSTALLFAILED;
        std::vector<suldownloaderdata::DownloadedProduct> downloadedProducts;
        auto metadata = createTestProductMetaData();
        metadata.setLine("P1");
        downloadedProducts.push_back(createTestDownloadedProduct(metadata));
        downloadedProducts[0].setError(wErr);
        downloadedProducts[0].setProductIsBeingUninstalled(true);

        wErr.Description = "Error P2";
        wErr.status = RepositoryStatus::INSTALLFAILED;

        metadata.setLine("P2");
        downloadedProducts.push_back(createTestDownloadedProduct(metadata));
        downloadedProducts[1].setError(wErr);

        suldownloaderdata::SubProducts products;
        auto version = metadata.getVersion(); // they all have the same version
        products.push_back({ "P1", version });
        products.push_back({ "P2", version });

        suldownloaderdata::SubscriptionInfo subscriptionInfo;
        subscriptionInfo.rigidName = "CS1";
        subscriptionInfo.version = version;
        subscriptionInfo.subProducts = products;
        std::vector<suldownloaderdata::SubscriptionInfo> subscriptions;
        subscriptions.push_back(subscriptionInfo);
        return std::make_pair(downloadedProducts, subscriptions);
    }
};

TEST_F(DownloadReportTest, fromReportWithReportCreatedFromErrorDescriptionShouldNotThrow)
{
    auto report = DownloadReportBuilder::Report("Test Failed example");

    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F(DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldReportSuccess)
{
    MockSdds3Repository mockSdds3Repository;
    RepositoryError error;
    error.Description = "";

    auto metadata = createTestProductMetaData();
    SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> products;
    products.push_back(downloadedProduct);

    EXPECT_CALL(mockSdds3Repository, hasError()).WillOnce(Return(false));
    EXPECT_CALL(mockSdds3Repository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockSdds3Repository, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(products)));
    EXPECT_CALL(mockSdds3Repository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(mockSdds3Repository, timeTracker);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::SUCCESS);
    checkReportValue(report, metadata);
    ASSERT_EQ(report.getProducts().size(), 1);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), "");

    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F(DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldReportFailedOnError)
{
    MockSdds3Repository mockSdds3Repository;
    std::string errorString = "Some Error";
    RepositoryError error;
    error.Description = errorString;
    error.status = RepositoryStatus::DOWNLOADFAILED;

    auto metadata = createTestProductMetaData();

    SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> products;

    products.push_back(downloadedProduct);

    EXPECT_CALL(mockSdds3Repository, hasError()).WillOnce(Return(true));
    EXPECT_CALL(mockSdds3Repository, getError()).WillOnce(Return(error));
    EXPECT_CALL(mockSdds3Repository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockSdds3Repository, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(products)));
    EXPECT_CALL(mockSdds3Repository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(mockSdds3Repository, timeTracker);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::DOWNLOADFAILED);
    checkReportValue(report, metadata);
    EXPECT_EQ(report.getProducts()[0].productStatus, ProductReport::ProductStatus::SyncFailed);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F(
    DownloadReportTest,
    fromReportWarehouseRepositoryAndTimeTrackerShouldReportSyncFailedForAllProductsOnMissingPackage)
{
    MockSdds3Repository mockSdds3Repository;
    std::string errorString = "Some Error";
    RepositoryError error;
    error.Description = errorString;
    error.status = RepositoryStatus::PACKAGESOURCEMISSING;

    auto metadata = createTestProductMetaData();

    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);

    EXPECT_CALL(mockSdds3Repository, hasError()).WillOnce(Return(true));
    EXPECT_CALL(mockSdds3Repository, getError()).WillOnce(Return(error));
    EXPECT_CALL(mockSdds3Repository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockSdds3Repository, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(products)));
    EXPECT_CALL(mockSdds3Repository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(mockSdds3Repository, timeTracker);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::PACKAGESOURCEMISSING);
    checkReportValue(report, metadata);
    EXPECT_EQ(report.getProducts()[0].productStatus, ProductReport::ProductStatus::SyncFailed);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F(DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldReportNoProductsOnAUnspecifiedError)
{
    MockSdds3Repository mockSdds3Repository;
    std::string errorString = "Some Error";
    RepositoryError error;
    error.Description = errorString;
    error.status = RepositoryStatus::UNSPECIFIED;

    auto report = DownloadReportBuilder::Report(errorString);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::UNSPECIFIED);
    EXPECT_EQ(report.getProducts().size(), 0);
    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F(DownloadReportTest, fromReportProductsAndTimeTrackerShouldReportInstalledFailedWhenProductHasErrors)
{
    std::string errorString = "Update failed";
    RepositoryError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(
        "sourceurl",
        products,
        {},
        subscriptionsFromProduct(products),
        &timeTracker,
        DownloadReportBuilder::VerifyState::VerifyCorrect,
        false);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    checkReportValue(report, metadata);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F(DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldCreateAValidReportForSuccess)
{
    MockSdds3Repository mockSdds3Repository;
    RepositoryError error;
    error.Description = "";

    auto metadata = createTestProductMetaData();
    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);

    EXPECT_CALL(mockSdds3Repository, hasError()).WillOnce(Return(false));
    EXPECT_CALL(mockSdds3Repository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockSdds3Repository, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(products)));
    EXPECT_CALL(mockSdds3Repository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(mockSdds3Repository, timeTracker);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::SUCCESS);
    checkReportValue(report, metadata);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), "");

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldCreateAValidReportForFailed)
{
    MockSdds3Repository mockSdds3Repository;

    std::string errorString = "Failed";

    RepositoryError error;
    error.Description = errorString;
    error.status = RepositoryStatus::DOWNLOADFAILED;

    auto metadata = createTestProductMetaData();
    SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);
    std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> products;
    products.push_back(downloadedProduct);

    EXPECT_CALL(mockSdds3Repository, hasError()).WillOnce(Return(true));
    EXPECT_CALL(mockSdds3Repository, getError()).WillOnce(Return(error));
    EXPECT_CALL(mockSdds3Repository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockSdds3Repository, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(products)));
    EXPECT_CALL(mockSdds3Repository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(mockSdds3Repository, timeTracker);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::DOWNLOADFAILED);
    checkReportValue(report, metadata);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());
    EXPECT_EQ(report.getProducts()[0].productStatus, ProductReport::ProductStatus::SyncFailed);

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenFails)
{
    std::string errorString = "Update failed";
    RepositoryError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> products;

    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(
        "sourceurl",
        products,
        {},
        subscriptionsFromProduct(products),
        &timeTracker,
        DownloadReportBuilder::VerifyState::VerifyCorrect,
        false);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    checkReportValue(report, metadata);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenFailsWithMultipleProducts)
{
    std::string errorString = "Update failed";
    RepositoryError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    auto metadata2 = createTestProductMetaData();
    metadata2.setLine("ProductLine2");
    metadata2.setName("Linux2");

    DownloadedProduct downloadedProduct2 = createTestDownloadedProduct(metadata2);

    downloadedProduct2.setError(error);

    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);
    products.push_back(downloadedProduct2);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(
        "sourceurl",
        products,
        {},
        subscriptionsFromProduct(products),
        &timeTracker,
        DownloadReportBuilder::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    EXPECT_EQ(report.getProducts().size(), 2);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenFailsNoProducts)
{
    auto metadata = createTestProductMetaData();

    TimeTracker timeTracker = createTimeTracker();

    std::vector<DownloadedProduct> products;

    auto report = DownloadReportBuilder::Report(
        "sourceurl",
        products,
        {},
        subscriptionsFromProduct(products),
        &timeTracker,
        DownloadReportBuilder::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::DOWNLOADFAILED);

    EXPECT_EQ(report.getProducts().size(), 0);

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutputWithNoProducts(report, jsonString);
}

TEST_F(
    DownloadReportTest,
    fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenInstallAndUninstallProductSucceeds)
{
    std::string errorString; // = "";
    RepositoryError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    auto metadata2 = createTestProductMetaData();
    metadata2.setLine("ProductLine2");
    metadata2.setName("Linux2");

    DownloadedProduct downloadedProduct2 = createTestDownloadedProduct(metadata2);

    downloadedProduct2.setError(error);

    auto metadata3 = createTestUninstalledProductMetaData();
    metadata3.setLine("ProductLine3");

    DownloadedProduct uninstalledProduct = createTestUninstalledProduct(metadata3);

    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);
    products.push_back(downloadedProduct2);
    products.push_back(uninstalledProduct);

    TimeTracker timeTracker = createTimeTracker();

    std::vector<ProductInfo> warehouseComponents;
    warehouseComponents.push_back(ProductInfo{ downloadedProduct.getLine(),
                                               downloadedProduct.getProductMetadata().getName(),
                                               downloadedProduct.getProductMetadata().getVersion(),
                                               downloadedProduct.getProductMetadata().getVersion() });

    auto report = DownloadReportBuilder::Report(
        "sophosurl",
        products,
        warehouseComponents,
        subscriptionsFromProduct(products),
        &timeTracker,
        DownloadReportBuilder::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::SUCCESS);

    EXPECT_STREQ(report.getDescription().c_str(), "");

    EXPECT_EQ(report.getProducts().size(), 3);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    EXPECT_PRED_FORMAT2(listProductInfoIsEquivalent, warehouseComponents, report.getRepositoryComponents());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(
    DownloadReportTest,
    fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenInstallProductSucceedsAndUninstallProductFails)
{
    std::string errorString; // = "";
    RepositoryError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    auto metadata2 = createTestProductMetaData();
    metadata2.setLine("ProductLine2");
    metadata2.setName("Linux2");

    DownloadedProduct downloadedProduct2 = createTestDownloadedProduct(metadata2);

    downloadedProduct2.setError(error);

    auto metadata3 = createTestUninstalledProductMetaData();
    metadata3.setLine("ProductLine3");

    std::string uninstallErrorString = "Uninstall Error";
    RepositoryError uninstallError;
    uninstallError.Description = uninstallErrorString;

    DownloadedProduct uninstalledProduct = createTestUninstalledProduct(metadata3);
    uninstalledProduct.setError(uninstallError);
    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);
    products.push_back(downloadedProduct2);
    products.push_back(uninstalledProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(
        "sophosurl",
        products,
        {},
        subscriptionsFromProduct(products),
        &timeTracker,
        DownloadReportBuilder::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::UNINSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), "Uninstall failed");

    EXPECT_EQ(report.getProducts().size(), 3);

    EXPECT_STREQ(report.getProducts()[2].errorDescription.c_str(), uninstallErrorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(
    DownloadReportTest,
    fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenInstallProductsFailAndUninstallProductFailsWithCorrectWHStatus)
{
    std::string errorString = "Install Failed";
    RepositoryError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    auto metadata2 = createTestProductMetaData();
    metadata2.setLine("ProductLine2");
    metadata2.setName("Linux2");

    DownloadedProduct downloadedProduct2 = createTestDownloadedProduct(metadata2);

    downloadedProduct2.setError(error);

    auto metadata3 = createTestUninstalledProductMetaData();
    metadata3.setLine("ProductLine3");

    std::string uninstallErrorString = "Uninstall Error";
    RepositoryError uninstallError;
    uninstallError.Description = uninstallErrorString;

    DownloadedProduct uninstalledProduct = createTestUninstalledProduct(metadata3);
    uninstalledProduct.setError(uninstallError);
    std::vector<DownloadedProduct> products;

    products.push_back(uninstalledProduct);
    products.push_back(downloadedProduct);
    products.push_back(downloadedProduct2);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReportBuilder::Report(
        "sophosurl",
        products,
        {},
        subscriptionsFromProduct(products),
        &timeTracker,
        DownloadReportBuilder::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), RepositoryStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), "Update failed");

    EXPECT_EQ(report.getProducts().size(), 3);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), uninstallErrorString.c_str());
    EXPECT_STREQ(report.getProducts()[1].errorDescription.c_str(), errorString.c_str());
    EXPECT_STREQ(report.getProducts()[2].errorDescription.c_str(), errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(DownloadReportTest, shouldExtractTheWarehouseSubComponents)
{
    std::string serializedReportWithSubComponents{ R"sophos({
    "finishTime": "20180822 121220",
    "status": "SUCCESS",
    "sulError": "",
    "products": [
        {
            "errorDescription": "",
            "rigidName": "ServerProtectionLinux-Base",
            "downloadVersion": "0.5.0",
            "productStatus": "UPTODATE",
            "productName": "ServerProtectionLinux-Base#0.5.0"
        },
        {
            "errorDescription": "",
            "rigidName": "ServerProtectionLinux-Plugin",
            "downloadVersion": "0.5.0",
            "productStatus": "UPTODATE",
            "productName": "ServerProtectionLinux-Plugin#0.5"
        }
    ],
    "warehouseComponents":[
        { "rigidName" : "rn1", "productName": "p1", "installedVersion":"v1", "warehouseVersion":"v1"},
        { "rigidName" : "rn2", "productName": "p2", "installedVersion":"v2", "warehouseVersion":"v2"}
    ],
    "startTime": "20180822 121220",
    "errorDescription": "",
    "urlSource": "Sophos",
})sophos" };
    DownloadReport report = DownloadReport::toReport(serializedReportWithSubComponents);
    const std::vector<ProductInfo>& warehouseComponents = report.getRepositoryComponents();
    EXPECT_EQ(warehouseComponents.size(), 2);
    std::vector<ProductInfo> expected = { { "rn1", "p1", "v1", "v1" }, { "rn2", "p2", "v2", "v2" } };
    EXPECT_PRED_FORMAT2(listProductInfoIsEquivalent, expected, warehouseComponents);
    std::string reSerialize = DownloadReport::fromReport(report);
    DownloadReport reportAgain = DownloadReport::toReport(reSerialize);
    EXPECT_PRED_FORMAT2(listProductInfoIsEquivalent, expected, reportAgain.getRepositoryComponents());
}

TEST_F(
    DownloadReportTest,
    combineProductsAndSubscriptions_ShouldReconcileSubscriptionsAndReportTheSubscriptionInsteadOftheUnderliningProducts)
{
    auto [downloadedProducts, subscriptions] = componentSuiteSubscriptionReferencingTwoProducts();

    auto reportProducts = DownloadReportBuilder::combineProductsAndSubscriptions(
        downloadedProducts, subscriptions, RepositoryStatus::SUCCESS);

    // given that there is an uninstallation, the uninstalled products will also be reported.
    ASSERT_EQ(reportProducts.size(), 2);
    EXPECT_EQ(reportProducts[0].rigidName, "P1");
    EXPECT_EQ(reportProducts[0].productStatus, ProductReport::ProductStatus::UninstallFailed);
    EXPECT_EQ(reportProducts[0].errorDescription, "Error P1");

    EXPECT_EQ(reportProducts[1].rigidName, "CS1");
    EXPECT_EQ(reportProducts[1].productStatus, ProductReport::ProductStatus::InstallFailed);
    EXPECT_EQ(reportProducts[1].errorDescription, "Error P1. Error P2");
}

TEST_F(
    DownloadReportTest,
    combineProductsAndSubscriptions_ShouldIgnoreProductsNotMatchingSubscriptionThatHasNoReferenceFromTheComponentSuite)
{
    auto [downloadedProducts, subscriptions] = componentSuiteSubscriptionReferencingTwoProducts();

    // setting the subscription as only a single product P1
    // the products still have 2 products, but the report products will only report on the one has a subscription
    // matching
    subscriptions[0].rigidName = "P1";
    subscriptions[0].subProducts.clear();

    auto reportProducts = DownloadReportBuilder::combineProductsAndSubscriptions(
        downloadedProducts, subscriptions, RepositoryStatus::SUCCESS);

    ASSERT_EQ(reportProducts.size(), 1);
    EXPECT_EQ(reportProducts[0].rigidName, "P1");
    EXPECT_EQ(reportProducts[0].productStatus, ProductReport::ProductStatus::UninstallFailed);
    EXPECT_EQ(reportProducts[0].errorDescription, "Error P1");
}

TEST_F(
    DownloadReportTest,
    combineProductsAndSubscriptions_ShouldReportOnAllProductsIfTheyAreMarkedAsFirstLevelSubscription)
{
    auto [downloadedProducts, subscriptions] = componentSuiteSubscriptionReferencingTwoProducts();

    suldownloaderdata::SubscriptionInfo subscriptionInfo;
    auto version = subscriptions[0].version;
    subscriptionInfo.rigidName = "P1";
    subscriptionInfo.version = version;
    subscriptions.clear();
    subscriptions.push_back(subscriptionInfo);
    subscriptionInfo.rigidName = "P2";
    subscriptionInfo.version = version;
    subscriptions.push_back(subscriptionInfo);

    // the subscriptions have 2 entries matching the downloaded entries. Both will be reported in the combined report
    auto reportProducts = DownloadReportBuilder::combineProductsAndSubscriptions(
        downloadedProducts, subscriptions, RepositoryStatus::SUCCESS);

    ASSERT_EQ(reportProducts.size(), 2);
    EXPECT_EQ(reportProducts[0].rigidName, "P1");
    EXPECT_EQ(reportProducts[1].rigidName, "P2");
    EXPECT_EQ(reportProducts[1].productStatus, ProductReport::ProductStatus::InstallFailed);
    EXPECT_EQ(reportProducts[1].errorDescription, "Error P2");
}

TEST_F(DownloadReportTest, shouldNotThrowOnValidReportWhenAnUnkownFieldIsPresent)
{
    std::string serializedReportWithSubComponents{ R"sophos({
 "startTime": "20200608 162035",
 "finishTime": "20200608 162106",
 "syncTime": "20200608 162106",
 "status": "SUCCESS",
 "sulError": "",
 "errorDescription": "",
 "urlSource": "https://ostia.eng.sophos/latest/sspl-warehouse/master",
 "products": [
  {
   "rigidName": "ServerProtectionLinux-Base",
   "productName": "Sophos Linux Base",
   "downloadVersion": "1.0.0",
   "errorDescription": "",
   "productStatus": "UPGRADED"
  }
 ],
 "warehouseComponents": [
  {
   "rigidName": "ServerProtectionLinux-Base",
   "productName": "Sophos Linux Base",
   "installedVersion": "1.0.0"
  }
 ],
 "aNewEntryThatWasNotKnownBefore": [
  {
   "rigidName": "ServerProtectionLinux-Base",
   "version": "1.0.0"
  }
 ] 
})sophos" };
    auto report = DownloadReport::toReport(serializedReportWithSubComponents);
    EXPECT_EQ(report.getProducts().size(), 1);
}

TEST_F(DownloadReportTest, FromReportToReportGivesBackExpectedResults)
{
    // Set up a DownloadReport, serialise it using fromReport, and deserialise using toReport

    ProductMetadata base;
    base.setDefaultHomePath("ServerProtectionLinux-Base-component");
    base.setLine("ServerProtectionLinux-Base-component");
    base.setName("ServerProtectionLinux-Base-component-Product");
    base.setVersion("10.2.3");
    base.setTags({ { "RECOMMENDED", "10", "Base-label" } });

    ProductMetadata plugin;
    plugin.setDefaultHomePath("ServerProtectionLinux-Plugin-EDR");
    plugin.setLine("ServerProtectionLinux-Plugin-EDR");
    plugin.setName("ServerProtectionLinux-Plugin-EDR-Product");
    plugin.setVersion("10.3.5");
    plugin.setTags({ { "RECOMMENDED", "10", "Plugin-label" } });
    plugin.setFeatures({ "feature1", "feature2" });
    plugin.setSubProduts({ { "line1", "version1" }, { "line2", "version2" } });

    DownloadedProduct baseProduct{ base };
    baseProduct.setDistributePath(
        "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    baseProduct.setError({ "description1", "description2", RepositoryStatus::INSTALLFAILED });
    baseProduct.setForceProductReinstall(true);
    baseProduct.setProductHasChanged(true);
    baseProduct.setProductIsBeingUninstalled(true);
    baseProduct.setProductWillBeDowngraded(true);

    DownloadedProduct edrProduct{ plugin };
    edrProduct.setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    edrProduct.setError({ "description3", "description4", RepositoryStatus::UNSPECIFIED });
    edrProduct.setForceProductReinstall(true);
    edrProduct.setProductHasChanged(true);
    edrProduct.setProductIsBeingUninstalled(true);
    edrProduct.setProductWillBeDowngraded(true);

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(1));
    timeTracker.setFinishedTime(std::time_t(2));

    std::vector<DownloadedProduct> downloadedProducts{ baseProduct, edrProduct };
    std::vector<ProductInfo> components{ { "rigid name", "product name", "version", "installed version" } };
    std::vector<SubscriptionInfo> subscriptionsToALCStatus{ { "subscription rigid name",
                                                              "subscription version",
                                                              { { "subscription line1", "subscription version1" },
                                                                { "subscription line2", "subscription version2" } } } };

    // baseDowngrade is not serialised/deserialised, and hence set to false
    const auto downloadReport = DownloadReportBuilder::Report(
        "source url",
        downloadedProducts,
        components,
        subscriptionsToALCStatus,
        &timeTracker,
        DownloadReportBuilder::VerifyState::VerifyCorrect,
        true,
        false);

    const auto serialised = DownloadReport::fromReport(downloadReport);

    checkJsonOutput(downloadReport, serialised);

    const auto deserialised = DownloadReport::toReport(serialised);
    EXPECT_EQ(deserialised, downloadReport);
}

TEST_F(DownloadReportTest, ProcessedReportIsNotSerialised)
{
    TimeTracker timeTracker;
    auto downloadReport = DownloadReportBuilder::Report(
        "source url", {}, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

    EXPECT_TRUE(DownloadReport::toReport(DownloadReport::fromReport(downloadReport)) == downloadReport);

    downloadReport.setProcessedReport(true);
    EXPECT_FALSE(DownloadReport::toReport(DownloadReport::fromReport(downloadReport)) == downloadReport);
}

TEST_F(DownloadReportTest, BaseDowngradeIsNotSerialised)
{
    TimeTracker timeTracker;

    const auto downloadReport = DownloadReportBuilder::Report(
        "source url", {}, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect, false, false);
    EXPECT_TRUE(DownloadReport::toReport(DownloadReport::fromReport(downloadReport)) == downloadReport);

    const auto downloadReport2 = DownloadReportBuilder::Report(
        "source url", {}, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect, false, true);
    EXPECT_FALSE(DownloadReport::toReport(DownloadReport::fromReport(downloadReport)) == downloadReport2);
}
