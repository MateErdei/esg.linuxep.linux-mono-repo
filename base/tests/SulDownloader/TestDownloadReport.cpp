/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockWarehouseRepository.h"
#include "TestWarehouseHelper.h"

#include <Common/UtilityImpl/TimeUtils.h>
#include <SulDownloader/WarehouseRepositoryFactory.h>
#include <SulDownloader/suldownloaderdata/DownloadReport.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <SulDownloader/suldownloaderdata/TimeTracker.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Common::UtilityImpl;

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class DownloadReportTest : public ::testing::Test
{
public:
    void SetUp() override {}

    void TearDown() override {}

    SulDownloader::suldownloaderdata::ProductMetadata createTestProductMetaData()
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

    SulDownloader::suldownloaderdata::DownloadedProduct createTestDownloadedProduct(
        SulDownloader::suldownloaderdata::ProductMetadata& metadata)
    {
        SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct(metadata);

        // set default data for tests
        downloadedProduct.setProductHasChanged(false);
        downloadedProduct.setDistributePath("/tmp");

        return downloadedProduct;
    }

    SulDownloader::suldownloaderdata::ProductMetadata createTestUninstalledProductMetaData()
    {
        SulDownloader::suldownloaderdata::ProductMetadata metadata;

        metadata.setLine("UninstalledProductLine1");

        return metadata;
    }

    SulDownloader::suldownloaderdata::DownloadedProduct createTestUninstalledProduct(
        SulDownloader::suldownloaderdata::ProductMetadata& metadata)
    {
        SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct(metadata);

        // set default data for tests
        downloadedProduct.setProductIsBeingUninstalled(true);

        return downloadedProduct;
    }

    TimeTracker createTimeTracker()
    {
        TimeTracker timeTracker;

        time_t currentTime;
        currentTime = TimeUtils::getCurrTime();

        timeTracker.setStartTime(currentTime - 30);
        timeTracker.setFinishedTime(currentTime - 1);
        return timeTracker;
    }

    void checkReportValue(DownloadReport& report, SulDownloader::suldownloaderdata::ProductMetadata& metadata)
    {
        EXPECT_EQ(report.getProducts().size(), 1);

        EXPECT_STREQ(report.getProducts()[0].rigidName.c_str(), metadata.getLine().c_str());
        EXPECT_STREQ(report.getProducts()[0].name.c_str(), metadata.getName().c_str());
        EXPECT_STREQ(report.getProducts()[0].downloadedVersion.c_str(), metadata.getVersion().c_str());
    }

    void checkJsonOutput(DownloadReport& report, std::string& jsonString)
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

        valueToFind = "syncTime\": \"" + report.getSyncTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "status\": \"" + SulDownloader::suldownloaderdata::toString(report.getStatus());
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "sulError\": \"" + report.getSulError();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "errorDescription\": \"" + report.getDescription();
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
    }

    void checkJsonOutputWithNoProducts(DownloadReport& report, std::string& jsonString)
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

        valueToFind = "syncTime\": \"" + report.getSyncTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "status\": \"" + SulDownloader::suldownloaderdata::toString(report.getStatus());
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "sulError\": \"" + report.getSulError();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "errorDescription\": \"" + report.getDescription();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        // double check to ensure we have something to loop through.
        EXPECT_EQ(report.getProducts().size(), 0);
    }
};

TEST_F(DownloadReportTest, fromReportWithReportCreatedFromErrorDescriptionShouldNotThrow) // NOLINT
{
    auto report = DownloadReport::Report("Test Failed example");

    EXPECT_NO_THROW(DownloadReport::fromReport(report)); // NOLINT
}

TEST_F(DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldReportSuccess) // NOLINT
{
    MockWarehouseRepository mockWarehouseRepository;
    WarehouseError error;
    error.Description = "";

    auto metadata = createTestProductMetaData();
    SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> products;
    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(false));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockWarehouseRepository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::SUCCESS);
    checkReportValue(report, metadata);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), "");

    EXPECT_NO_THROW(DownloadReport::fromReport(report)); // NOLINT
}

TEST_F(DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldReportFailedOnError) // NOLINT
{
    MockWarehouseRepository mockWarehouseRepository;
    std::string errorString = "Some Error";
    WarehouseError error;
    error.Description = errorString;
    error.status = WarehouseStatus::DOWNLOADFAILED;

    auto metadata = createTestProductMetaData();

    SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> products;

    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(true));
    EXPECT_CALL(mockWarehouseRepository, getError()).WillOnce(Return(error));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockWarehouseRepository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::DOWNLOADFAILED);
    checkReportValue(report, metadata);
    EXPECT_EQ(report.getProducts()[0].productStatus, ProductReport::ProductStatus::SyncFailed);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    EXPECT_NO_THROW(DownloadReport::fromReport(report)); // NOLINT
}

TEST_F( // NOLINT
    DownloadReportTest,
    fromReportWarehouseRepositoryAndTimeTrackerShouldReportSyncFailedForAllProductsOnMissingPackage)
{
    MockWarehouseRepository mockWarehouseRepository;
    std::string errorString = "Some Error";
    WarehouseError error;
    error.Description = errorString;
    error.status = WarehouseStatus::PACKAGESOURCEMISSING;

    auto metadata = createTestProductMetaData();

    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(true));
    EXPECT_CALL(mockWarehouseRepository, getError()).WillOnce(Return(error));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockWarehouseRepository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::PACKAGESOURCEMISSING);
    checkReportValue(report, metadata);
    EXPECT_EQ(report.getProducts()[0].productStatus, ProductReport::ProductStatus::SyncFailed);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    EXPECT_NO_THROW(DownloadReport::fromReport(report)); // NOLINT
}

TEST_F( // NOLINT
    DownloadReportTest,
    fromReportWarehouseRepositoryAndTimeTrackerShouldReportNoProductsOnAUnspecifiedError)
{
    MockWarehouseRepository mockWarehouseRepository;
    std::string errorString = "Some Error";
    WarehouseError error;
    error.Description = errorString;
    error.status = WarehouseStatus::UNSPECIFIED;

    auto report = DownloadReport::Report(errorString);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::UNSPECIFIED);
    EXPECT_EQ(report.getProducts().size(), 0);
    EXPECT_NO_THROW(DownloadReport::fromReport(report)); // NOLINT
}

TEST_F(DownloadReportTest, fromReportProductsAndTimeTrackerShouldReportInstalledFailedWhenProductHasErrors) // NOLINT
{
    std::string errorString = "Update failed";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report =
        DownloadReport::Report("sourceurl", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    checkReportValue(report, metadata);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    EXPECT_NO_THROW(DownloadReport::fromReport(report)); // NOLINT
}

TEST_F(DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldCreateAValidReportForSuccess) // NOLINT
{
    MockWarehouseRepository mockWarehouseRepository;
    WarehouseError error;
    error.Description = "";

    auto metadata = createTestProductMetaData();
    DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(false));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockWarehouseRepository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::SUCCESS);
    checkReportValue(report, metadata);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), "");

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldCreateAValidReportForFailed) // NOLINT
{
    MockWarehouseRepository mockWarehouseRepository;

    std::string errorString = "Failed";

    WarehouseError error;
    error.Description = errorString;
    error.status = WarehouseStatus::DOWNLOADFAILED;

    auto metadata = createTestProductMetaData();
    SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);
    std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> products;
    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(true));
    EXPECT_CALL(mockWarehouseRepository, getError()).WillOnce(Return(error));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mockWarehouseRepository, getSourceURL()).WillOnce(Return(""));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::DOWNLOADFAILED);
    checkReportValue(report, metadata);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());
    EXPECT_EQ(report.getProducts()[0].productStatus, ProductReport::ProductStatus::SyncFailed);

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenFails) // NOLINT
{
    std::string errorString = "Update failed";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> products;

    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report =
        DownloadReport::Report("sourceurl", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    checkReportValue(report, metadata);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F( // NOLINT
    DownloadReportTest,
    fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenFailsWithMultipleProducts)
{
    std::string errorString = "Update failed";
    WarehouseError error;
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

    auto report =
        DownloadReport::Report("sourceurl", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    EXPECT_EQ(report.getProducts().size(), 2);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F(DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenFailsNoProducts) // NOLINT
{
    auto metadata = createTestProductMetaData();

    TimeTracker timeTracker = createTimeTracker();

    std::vector<DownloadedProduct> products;

    auto report =
        DownloadReport::Report("sourceurl", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::DOWNLOADFAILED);

    EXPECT_EQ(report.getProducts().size(), 0);

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutputWithNoProducts(report, jsonString);
}

TEST_F( // NOLINT
    DownloadReportTest,
    fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenInstallAndUninstallProductSucceeds)
{
    std::string errorString; // = "";
    WarehouseError error;
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
                                               downloadedProduct.getProductMetadata().getVersion() });

    std::vector<SubscriptionInfo> subscriptionComponents;
    subscriptionComponents.push_back({ downloadedProduct.getLine(), downloadedProduct.getProductMetadata().getVersion() });

    auto report = DownloadReport::Report(
        "sophosurl", products, warehouseComponents, subscriptionComponents, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::SUCCESS);

    EXPECT_STREQ(report.getDescription().c_str(), "");

    EXPECT_EQ(report.getProducts().size(), 3);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    EXPECT_PRED_FORMAT2(listProductInfoIsEquivalent, warehouseComponents, report.getWarehouseComponents());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F( // NOLINT
    DownloadReportTest,
    fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenInstallProductSucceedsAndUninstallProductFails)
{
    std::string errorString; // = "";
    WarehouseError error;
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
    WarehouseError uninstallError;
    uninstallError.Description = uninstallErrorString;

    DownloadedProduct uninstalledProduct = createTestUninstalledProduct(metadata3);
    uninstalledProduct.setError(uninstallError);
    std::vector<DownloadedProduct> products;

    products.push_back(downloadedProduct);
    products.push_back(downloadedProduct2);
    products.push_back(uninstalledProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report =
        DownloadReport::Report("sophosurl", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::UNINSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), "Uninstall failed");

    EXPECT_EQ(report.getProducts().size(), 3);

    EXPECT_STREQ(report.getProducts()[2].errorDescription.c_str(), uninstallErrorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F( // NOLINT
    DownloadReportTest,
    fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenInstallProductsFailAndUninstallProductFailsWithCorrectWHStatus)
{
    std::string errorString = "Install Failed";
    WarehouseError error;
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
    WarehouseError uninstallError;
    uninstallError.Description = uninstallErrorString;

    DownloadedProduct uninstalledProduct = createTestUninstalledProduct(metadata3);
    uninstalledProduct.setError(uninstallError);
    std::vector<DownloadedProduct> products;

    products.push_back(uninstalledProduct);
    products.push_back(downloadedProduct);
    products.push_back(downloadedProduct2);

    TimeTracker timeTracker = createTimeTracker();

    auto report =
        DownloadReport::Report("sophosurl", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
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
        { "rigidName" : "rn1", "productName": "p1", "installedVersion":"v1"},
        { "rigidName" : "rn2", "productName": "p2", "installedVersion":"v2"}
    ],
    "startTime": "20180822 121220",
    "errorDescription": "",
    "urlSource": "Sophos",
    "syncTime": "20180821 121220"
})sophos" };
    DownloadReport report = DownloadReport::toReport(serializedReportWithSubComponents);
    std::vector<suldownloaderdata::ProductInfo> warehouseComponents = report.getWarehouseComponents();
    EXPECT_EQ(warehouseComponents.size(), 2);
    std::vector<suldownloaderdata::ProductInfo> expected = { { "rn1", "p1", "v1" }, { "rn2", "p2", "v2" } };
    EXPECT_PRED_FORMAT2(listProductInfoIsEquivalent, expected, warehouseComponents);
    std::string reSerialize = DownloadReport::fromReport(report);
    DownloadReport reportAgain = DownloadReport::toReport(reSerialize);
    EXPECT_PRED_FORMAT2(listProductInfoIsEquivalent, expected, reportAgain.getWarehouseComponents());
}


TEST_F(DownloadReportTest, shouldExtractTheSubscriptionComponents)
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
        { "rigidName" : "rn1", "productName": "p1", "installedVersion":"v1"},
        { "rigidName" : "rn2", "productName": "p2", "installedVersion":"v2"}
    ],
    "subscriptionComponents":[
        { "rigidName" : "subrn1", "version": "v1.2"},
        { "rigidName" : "subrn2", "version": "v2.3"}
    ],
    "startTime": "20180822 121220",
    "errorDescription": "",
    "urlSource": "Sophos",
    "syncTime": "20180821 121220"
})sophos" };
    DownloadReport report = DownloadReport::toReport(serializedReportWithSubComponents);
    std::vector<suldownloaderdata::SubscriptionInfo> subscriptionComponents = report.getSubscriptionComponents();
    std::vector<suldownloaderdata::SubscriptionInfo> expected = { { "subrn1", "v1.2" }, { "subrn2", "v2.3" } };
    EXPECT_EQ( subscriptionComponents, expected );
    // serialization and deserialization preserves SubscriptionComponents
    auto rerialized = DownloadReport::fromReport(report); 
    DownloadReport report2 = DownloadReport::toReport(rerialized); 
    subscriptionComponents = report2.getSubscriptionComponents();
    EXPECT_EQ( subscriptionComponents, expected );
}

