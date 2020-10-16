/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "gmock/gmock.h"

#include <SulDownloader/suldownloaderdata/ConnectionSetup.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/IWarehouseRepository.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/WarehouseError.h>

using namespace ::testing;
using namespace SulDownloader;

class MockWarehouseRepository : public suldownloaderdata::IWarehouseRepository
{
public:
    MOCK_CONST_METHOD0(hasError, bool(void));
    MOCK_CONST_METHOD0(getError, suldownloaderdata::WarehouseError(void));
    MOCK_METHOD1(synchronize, void(suldownloaderdata::ProductSelection&));
    MOCK_METHOD0(distribute, void(void));
    MOCK_CONST_METHOD0(getProducts, std::vector<suldownloaderdata::DownloadedProduct>(void));
    MOCK_CONST_METHOD0(getSourceURL, std::string(void));

    MOCK_CONST_METHOD1(getProductDistributionPath, std::string(const suldownloaderdata::DownloadedProduct&));
    MOCK_CONST_METHOD0(listInstalledProducts, std::vector<suldownloaderdata::ProductInfo>(void));
    MOCK_CONST_METHOD0(
        listInstalledSubscriptions,
        std::vector<suldownloaderdata::SubscriptionInfo>(void));

    MOCK_METHOD3(tryConnect, bool(
                                 const suldownloaderdata::ConnectionSetup&,
                                 bool,
                                 const suldownloaderdata::ConfigurationData&
                                 ));
    MOCK_CONST_METHOD0(getLogs, SulLogsVector(void));
    MOCK_CONST_METHOD0(dumpLogs, void(void));

    MOCK_METHOD0(reset, void(void));
};
