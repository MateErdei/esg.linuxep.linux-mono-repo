/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "TestWarehouseHelper.h"
#include "SulDownloader/WarehouseRepositoryFactory.h"
#include <SulDownloader/suldownloaderdata/WarehouseError.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include "MockWarehouseRepository.h"

TEST( MockWarehouseRepositoryTest, DemonstrateMockWarehouse) //NOLINT
{
    MockWarehouseRepository mock;
    WarehouseError error;
    error.Description = "Nothing";
    error.status = SulDownloader::SUCCESS;
    suldownloaderdata::ConfigurationData configurationData({"https://sophos.com/warehouse"});
    suldownloaderdata::ProductGUID productGUID{"ProductName",true,false,"ReleaseTag","BaseVersion"};
    configurationData.addProductSelection(productGUID);
    SulDownloader::ProductSelection selection = SulDownloader::ProductSelection::CreateProductSelection(configurationData);

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

    ASSERT_EQ(mock.getProducts().at(0).getLine(),"");
}

TEST( MockWarehouseRepositoryTest, ReplaceWarehouseRepository) //NOLINT
{
    auto mockptr = new MockWarehouseRepository();
    MockWarehouseRepository& mock = *mockptr;

    TestWarehouseHelper helper;
    helper.replaceWarehouseCreator(
            [&mockptr](const suldownloaderdata::ConfigurationData & ){return suldownloaderdata::IWarehouseRepositoryPtr(mockptr);});
    EXPECT_CALL(mock, hasError()).WillOnce(Return(false)).WillOnce(Return(true));


    suldownloaderdata::ConfigurationData configurationData({"https://sophos.com/warehouse"});
    auto warehouseFromFactory = SulDownloader::WarehouseRepositoryFactory::instance().fetchConnectedWarehouseRepository(configurationData);
    ASSERT_FALSE(warehouseFromFactory->hasError());
    ASSERT_TRUE(warehouseFromFactory->hasError());
}
