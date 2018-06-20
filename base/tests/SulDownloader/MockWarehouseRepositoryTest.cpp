///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "TestWarehouseHelper.h"
#include "SulDownloader/WarehouseRepositoryFactory.h"
#include "SulDownloader/WarehouseError.h"
#include "SulDownloader/ProductSelection.h"
#include "SulDownloader/DownloadedProduct.h"
#include "SulDownloader/ConfigurationData.h"
#include "MockWarehouseRepository.h"

TEST( MockWarehouseRepositoryTest, DemonstrateMockWarehouse)
{
    MockWarehouseRepository mock;
    WarehouseError error;
    error.Description = "Nothing";
    ConfigurationData configurationData({"https://sophos.com/warehouse"});
    ProductGUID productGUID{"ProductName",true,false,"ReleaseTag","BaseVersion"};
    configurationData.addProductSelection(productGUID);
    SulDownloader::ProductSelection selection = SulDownloader::ProductSelection::CreateProductSelection(configurationData);

    SulDownloader::ProductMetadata metadata;
    SulDownloader::DownloadedProduct downloadedProduct(metadata);
    std::vector<SulDownloader::DownloadedProduct> products;
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

TEST( MockWarehouseRepositoryTest, ReplaceWarehouseRepository)
{
    MockWarehouseRepository * mockptr = new MockWarehouseRepository();
    MockWarehouseRepository & mock = *mockptr;

    TestWarehouseHelper helper;
    helper.replaceWarehouseCreator([&mockptr](const ConfigurationData & ){return std::unique_ptr<SulDownloader::IWarehouseRepository>(mockptr);});
    EXPECT_CALL(mock, hasError()).WillOnce(Return(false)).WillOnce(Return(true));


    ConfigurationData configurationData({"https://sophos.com/warehouse"});
    auto warehouseFromFactory = SulDownloader::WarehouseRepositoryFactory::instance().fetchConnectedWarehouseRepository(configurationData);
    ASSERT_FALSE(warehouseFromFactory->hasError());
    ASSERT_TRUE(warehouseFromFactory->hasError());
}
