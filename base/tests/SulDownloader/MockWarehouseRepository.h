/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "gmock/gmock.h"

#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/IWarehouseRepository.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/WarehouseError.h>

using namespace ::testing;
using namespace SulDownloader;

class MockWarehouseRepository : public SulDownloader::suldownloaderdata::IWarehouseRepository
{
public:
    MOCK_CONST_METHOD0(hasError, bool(void));
    MOCK_CONST_METHOD0(getError, suldownloaderdata::WarehouseError(void));
    MOCK_METHOD1(synchronize, void(SulDownloader::suldownloaderdata::ProductSelection&));
    MOCK_METHOD0(distribute, void(void));
    MOCK_CONST_METHOD0(getProducts, std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>(void));
    MOCK_CONST_METHOD0(getSourceURL, std::string(void));

    MOCK_CONST_METHOD1(getProductDistributionPath, std::string(const suldownloaderdata::DownloadedProduct&));
    MOCK_CONST_METHOD0(listInstalledProducts, std::vector<SulDownloader::suldownloaderdata::ProductInfo>(void));
    MOCK_CONST_METHOD0(
        listInstalledSubscriptions,
        std::vector<SulDownloader::suldownloaderdata::SubscriptionInfo>(void));
};
