/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "gtest/gtest_pred_impl.h"

#include "SulDownloader/DownloadReport.h"
#include "SulDownloader/SulDownloaderException.h"
#include "../Common/TestHelpers/TempDir.h"

#include "TestWarehouseHelper.h"
#include "SulDownloader/WarehouseRepositoryFactory.h"
#include "SulDownloader/WarehouseError.h"
#include "SulDownloader/ProductSelection.h"
#include "SulDownloader/DownloadedProduct.h"
#include "SulDownloader/ConfigurationData.h"
#include "SulDownloader/TimeTracker.h"
#include "MockWarehouseRepository.h"



using namespace SulDownloader;


class DownloadReportTest : public ::testing::Test
{

public:

    void SetUp() override
    {

    }

    void TearDown() override
    {

    }

    SulDownloader::ProductMetadata createTestProductMetaData()
    {
        SulDownloader::ProductMetadata metadata;

        metadata.setLine("ProductLine1");
        metadata.setDefaultHomePath("Linux");
        metadata.setVersion("1.1.1.1");
        metadata.setName("Linux Security");
        Tag tag("RECOMMENDED", "1", "RECOMMENDED");
        metadata.setTags({tag});

        return metadata;
    }

    SulDownloader::DownloadedProduct createTestDownloadedProduct(SulDownloader::ProductMetadata &metadata)
    {
        SulDownloader::DownloadedProduct downloadedProduct(createTestProductMetaData());

        // set default data for tests
        downloadedProduct.setProductHasChanged(false);
        downloadedProduct.setDistributePath("/tmp");
        downloadedProduct.setPostUpdateInstalledVersion("1.0.0.0");
        downloadedProduct.setPreUpdateInstalledVersion("1.0.0.0");

        return downloadedProduct;
    }

    TimeTracker createTimeTracker()
    {
        TimeTracker timeTracker;

        time_t currentTime;
        currentTime = TimeTracker::getCurrTime();

        timeTracker.setStartTime(currentTime - 30);
        timeTracker.setSyncTime(currentTime -20);
        timeTracker.setFinishedTime(currentTime - 1);
        return timeTracker;

    }

    void checkReportValue(DownloadReport &report, SulDownloader::ProductMetadata &metadata, SulDownloader::DownloadedProduct &downloadedProduct)
    {
        EXPECT_EQ(report.getProducts().size(), 1);

        EXPECT_STREQ(report.getProducts()[0].rigidName.c_str(),  metadata.getLine().c_str());
        EXPECT_STREQ(report.getProducts()[0].name.c_str(),  metadata.getName().c_str());
        EXPECT_STREQ(report.getProducts()[0].downloadedVersion.c_str(),  metadata.getVersion().c_str());
        EXPECT_STREQ(report.getProducts()[0].installedVersion.c_str(),  downloadedProduct.getPostUpdateInstalledVersion().c_str());
    }

    void checkJsonOutput(DownloadReport &report, std::string &jsonString)
    {
        //example jsonString being passed
        //"{\n \"startTime\": \"20180621 142342\",\n \"finishTime\": \"20180621 142411\",\n \"syncTime\": \"20180621 142352\",\n \"status\": \"SUCCESS\",\n \"sulError\": \"\",\n \"errorDescription\": \"\",\n \"products\": [\n  {\n   \"rigidName\": \"ProductLine1\",\n   \"productName\": \"Linux Security\",\n   \"installedVersion\": \"1.1.1.1\",\n   \"downloadVersion\": \"1.1.1.1\",\n   \"errorDescription\": \"\"\n  }\n ]\n}\n"

        std::string valueToFind;

        valueToFind = "startTime\": \"" + report.getStartTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "finishTime\": \"" + report.getFinishedTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "syncTime\": \"" + report.getSyncTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "status\": \"" + SulDownloader::toString(report.getStatus());
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "sulError\": \"" + report.getSulError();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "errorDescription\": \"" + report.getDescription();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        // double check to ensure we have something to loop through.
        EXPECT_NE(report.getProducts().size(), 0);

        for(auto & product : report.getProducts())
        {
            valueToFind = "rigidName\": \"" + product.rigidName;
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

            valueToFind = "productName\": \"" + product.name;
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

            valueToFind = "installedVersion\": \"" + product.installedVersion;
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

            valueToFind = "downloadVersion\": \"" + product.downloadedVersion;
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

            valueToFind = "errorDescription\": \"" + product.errorDescription;
            EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));
        }
    }

    void checkJsonOutputWithNoProducts(DownloadReport &report, std::string &jsonString)
    {
        //example jsonString being passed
        //"{\n \"startTime\": \"20180621 142342\",\n \"finishTime\": \"20180621 142411\",\n \"syncTime\": \"20180621 142352\",\n \"status\": \"SUCCESS\",\n \"sulError\": \"\",\n \"errorDescription\": \"\",\n \"products\": [\n { }\n ]\n}\n"

        std::string valueToFind;

        valueToFind = "startTime\": \"" + report.getStartTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "finishTime\": \"" + report.getFinishedTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "syncTime\": \"" + report.getSyncTime();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "status\": \"" + SulDownloader::toString(report.getStatus());
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "sulError\": \"" + report.getSulError();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        valueToFind = "errorDescription\": \"" + report.getDescription();
        EXPECT_THAT(jsonString, ::testing::HasSubstr(valueToFind));

        // double check to ensure we have something to loop through.
        EXPECT_EQ(report.getProducts().size(), 0);
    }

};

TEST_F( DownloadReportTest, fromReportWithReportCreatedFromErrorDescriptionShouldNotThrow)
{
    auto report = DownloadReport::Report("Test Failed example");

    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F( DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldReportSuccess)
{
    MockWarehouseRepository mockWarehouseRepository;
    WarehouseError error;
    error.Description = "";

    auto metadata = createTestProductMetaData();
    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setPostUpdateInstalledVersion("1.1.1.1");
    std::vector<SulDownloader::DownloadedProduct> products;
    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(false));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::SUCCESS);
    checkReportValue(report, metadata, downloadedProduct);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  "");

    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F( DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldReportInstallFailedWhenReportedVersionsDoNotMatch)
{
    MockWarehouseRepository mockWarehouseRepository;
    std::string errorString = "";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();
    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    std::vector<SulDownloader::DownloadedProduct> products;
    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(false));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    checkReportValue(report, metadata, downloadedProduct);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());

    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}


TEST_F( DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldReportFailedOnError)
{
    MockWarehouseRepository mockWarehouseRepository;
    std::string errorString = "Some Error";
    WarehouseError error;
    error.Description = errorString;
    error.status = WarehouseStatus::DOWNLOADFAILED;

    auto metadata = createTestProductMetaData();

    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    std::vector<SulDownloader::DownloadedProduct> products;

    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(true));
    EXPECT_CALL(mockWarehouseRepository, getError()).WillOnce(Return(error));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));


    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::DOWNLOADFAILED);
    checkReportValue(report, metadata, downloadedProduct);

    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldReportSuccessWhenReportedVersionsMatch)
{
    WarehouseError error;
    error.Description = "";

    auto metadata = createTestProductMetaData();

    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setPostUpdateInstalledVersion("1.1.1.1");

    downloadedProduct.setError(error);

    std::vector<SulDownloader::DownloadedProduct> products;


    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::SUCCESS);
    EXPECT_STREQ(report.getDescription().c_str(), "");
    checkReportValue(report, metadata, downloadedProduct);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  "");

    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldReportInstalledFailedWhenProductHasErrors)
{
    std::string errorString = "Update failed";
    WarehouseError error;
    error.Description =errorString;

    auto metadata = createTestProductMetaData();

    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setPostUpdateInstalledVersion("1.1.1.1");

    downloadedProduct.setError(error);

    std::vector<SulDownloader::DownloadedProduct> products;


    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    checkReportValue(report, metadata, downloadedProduct);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  errorString.c_str());

    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}


TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldReportInstalledFailedWhenReportedVersionsDoNotMatch)
{
    std::string errorString = "";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    std::vector<SulDownloader::DownloadedProduct> products;

    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);

    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    checkReportValue(report, metadata, downloadedProduct);

    EXPECT_NO_THROW(DownloadReport::fromReport(report));
}

TEST_F( DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldCreateAValidReportForSuccess)
{
    MockWarehouseRepository mockWarehouseRepository;
    WarehouseError error;
    error.Description = "";

    auto metadata = createTestProductMetaData();
    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setPostUpdateInstalledVersion("1.1.1.1");
    std::vector<SulDownloader::DownloadedProduct> products;


    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(false));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::SUCCESS);
    checkReportValue(report, metadata, downloadedProduct);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  "");

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F( DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldCreateAValidReportForFailed)
{
    MockWarehouseRepository mockWarehouseRepository;

    std::string errorString = "Failed";

    WarehouseError error;
    error.Description = errorString;
    error.status = WarehouseStatus::DOWNLOADFAILED;

    auto metadata = createTestProductMetaData();
    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setPostUpdateInstalledVersion("1.1.1.1");
    downloadedProduct.setError(error);
    std::vector<SulDownloader::DownloadedProduct> products;
    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(true));
    EXPECT_CALL(mockWarehouseRepository, getError()).WillOnce(Return(error));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::DOWNLOADFAILED);
    checkReportValue(report, metadata, downloadedProduct);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);

}

TEST_F( DownloadReportTest, fromReportWarehouseRepositoryAndTimeTrackerShouldCreateAValidReportForMissMatchedVersions)
{
    MockWarehouseRepository mockWarehouseRepository;

    std::string errorString = "";

    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();
    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);
    std::vector<SulDownloader::DownloadedProduct> products;
    products.push_back(downloadedProduct);

    EXPECT_CALL(mockWarehouseRepository, hasError()).WillOnce(Return(false));
    EXPECT_CALL(mockWarehouseRepository, getProducts()).WillOnce(Return(products));

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(mockWarehouseRepository, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    checkReportValue(report, metadata, downloadedProduct);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(), errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);

}


TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenReportedVersionsMatch)
{
    std::string errorString = "";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setPostUpdateInstalledVersion("1.1.1.1");

    downloadedProduct.setError(error);

    std::vector<SulDownloader::DownloadedProduct> products;


    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::SUCCESS);
    EXPECT_STREQ(report.getDescription().c_str(), "");
    checkReportValue(report, metadata, downloadedProduct);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}


TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenReportedVersionsDoNotMatch)
{
    std::string errorString = "";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);

    downloadedProduct.setError(error);

    std::vector<SulDownloader::DownloadedProduct> products;

    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), "");
    checkReportValue(report, metadata, downloadedProduct);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenFails)
{
    std::string errorString = "Update failed";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setPostUpdateInstalledVersion("1.1.1.1");
    downloadedProduct.setError(error);

    std::vector<SulDownloader::DownloadedProduct> products;

    products.push_back(downloadedProduct);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    checkReportValue(report, metadata, downloadedProduct);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenReportedVersionsMatchWithMultipleProducts)
{
    std::string errorString = "";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setPostUpdateInstalledVersion("1.1.1.1");
    downloadedProduct.setError(error);

    auto metadata2 = createTestProductMetaData();
    metadata2.setLine("ProductLine2");
    metadata.setName("Linux2");

    SulDownloader::DownloadedProduct downloadedProduct2 = createTestDownloadedProduct(metadata2);
    downloadedProduct2.setPostUpdateInstalledVersion("1.1.1.1");
    downloadedProduct2.setError(error);

    std::vector<SulDownloader::DownloadedProduct> products;


    products.push_back(downloadedProduct);
    products.push_back(downloadedProduct2);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::SUCCESS);
    EXPECT_STREQ(report.getDescription().c_str(), "");

    EXPECT_EQ(report.getProducts().size(), 2);

    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}


TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenReportedVersionsDoNotMatchWithMultipleProducts)
{
    std::string errorString = "";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();
    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setError(error);

    auto metadata2 = createTestProductMetaData();
    metadata2.setLine("ProductLine2");
    metadata.setName("Linux2");

    SulDownloader::DownloadedProduct downloadedProduct2 = createTestDownloadedProduct(metadata2);
    downloadedProduct2.setPostUpdateInstalledVersion("1.1.1.1");
    downloadedProduct2.setError(error);

    std::vector<SulDownloader::DownloadedProduct> products;


    products.push_back(downloadedProduct);
    products.push_back(downloadedProduct2);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), "");
    EXPECT_EQ(report.getProducts().size(), 2);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenFailsWithMultipleProducts)
{
    std::string errorString = "Update failed";
    WarehouseError error;
    error.Description = errorString;

    auto metadata = createTestProductMetaData();

    SulDownloader::DownloadedProduct downloadedProduct = createTestDownloadedProduct(metadata);
    downloadedProduct.setPostUpdateInstalledVersion("1.1.1.1");
    downloadedProduct.setError(error);

    auto metadata2 = createTestProductMetaData();
    metadata2.setLine("ProductLine2");
    metadata.setName("Linux2");

    SulDownloader::DownloadedProduct downloadedProduct2 = createTestDownloadedProduct(metadata2);
    downloadedProduct2.setPostUpdateInstalledVersion("1.1.1.1");
    downloadedProduct2.setError(error);

    std::vector<SulDownloader::DownloadedProduct> products;


    products.push_back(downloadedProduct);
    products.push_back(downloadedProduct2);

    TimeTracker timeTracker = createTimeTracker();

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::INSTALLFAILED);
    EXPECT_STREQ(report.getDescription().c_str(), errorString.c_str());
    EXPECT_EQ(report.getProducts().size(), 2);
    EXPECT_STREQ(report.getProducts()[0].errorDescription.c_str(),  errorString.c_str());

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutput(report, jsonString);
}

TEST_F( DownloadReportTest, fromReportProductsAndTimeTrackerShouldCreateAValidReportWhenFailsNoProducts)
{

    auto metadata = createTestProductMetaData();

    TimeTracker timeTracker = createTimeTracker();

    std::vector<SulDownloader::DownloadedProduct> products;

    auto report = DownloadReport::Report(products, timeTracker);

    EXPECT_EQ(report.getStatus(), WarehouseStatus::DOWNLOADFAILED);

    EXPECT_EQ(report.getProducts().size(), 0);

    auto jsonReport = DownloadReport::CodeAndSerialize(report);

    std::string jsonString = std::get<1>(jsonReport);

    checkJsonOutputWithNoProducts(report, jsonString);
}


