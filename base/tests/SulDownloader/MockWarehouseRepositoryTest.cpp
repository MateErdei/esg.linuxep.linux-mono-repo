/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockWarehouseRepository.h"

#include "TestWarehouseHelper.h"

#include "SulDownloader/WarehouseRepositoryFactory.h"

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/WarehouseError.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockWarehouseRepositoryTest: public LogOffInitializedTests{};

TEST_F(MockWarehouseRepositoryTest, DemonstrateMockWarehouse) // NOLINT
{
    MockWarehouseRepository mock;
    suldownloaderdata::WarehouseError error;
    error.Description = "Nothing";
    error.status = SulDownloader::suldownloaderdata::SUCCESS;
    suldownloaderdata::ConfigurationData configurationData({ "https://sophos.com/warehouse" });
    configurationData.setPrimarySubscription({ "ServerProtectionLinux-Base", "", "RECOMMENDED", "" });
    auto selection = SulDownloader::suldownloaderdata::ProductSelection::CreateProductSelection(configurationData);

    SulDownloader::suldownloaderdata::ProductMetadata metadata;
    SulDownloader::suldownloaderdata::DownloadedProduct downloadedProduct(metadata);
    std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> products;
    products.push_back(downloadedProduct);

    EXPECT_CALL(mock, hasError()).WillOnce(Return(false));
    EXPECT_CALL(mock, getError()).WillOnce(Return(error));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));

    ASSERT_FALSE(mock.hasError());
    ASSERT_EQ(mock.getError().Description, error.Description);
    mock.synchronize(selection);
    mock.distribute();

    ASSERT_EQ(mock.getProducts().at(0).getLine(), "");
}

TEST_F(MockWarehouseRepositoryTest, ReplaceWarehouseRepository) // NOLINT
{
    auto* mockptr = new MockWarehouseRepository();
    MockWarehouseRepository& mock = *mockptr;

    TestWarehouseHelper::replaceWarehouseCreator([&mockptr]() {
        return suldownloaderdata::IWarehouseRepositoryPtr(mockptr);
    });
    EXPECT_CALL(mock, hasError()).WillOnce(Return(false)).WillOnce(Return(true));

    suldownloaderdata::ConfigurationData configurationData({ "https://sophos.com/warehouse" });
    auto warehouseFromFactory =
        SulDownloader::WarehouseRepositoryFactory::instance().createWarehouseRepository();
    ASSERT_FALSE(warehouseFromFactory->hasError());
    ASSERT_TRUE(warehouseFromFactory->hasError());
}
