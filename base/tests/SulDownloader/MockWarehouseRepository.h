/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <SulDownloader/suldownloaderdata/IWarehouseRepository.h>
#include <SulDownloader/suldownloaderdata/WarehouseError.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>

using namespace ::testing;
using namespace SulDownloader;

class MockWarehouseRepository: public SulDownloader::suldownloaderdata::IWarehouseRepository
{
public:
    MOCK_CONST_METHOD0(hasError, bool(void));
    MOCK_CONST_METHOD0(getError, WarehouseError(void));
    MOCK_METHOD1(synchronize, void(SulDownloader::ProductSelection&));
    MOCK_METHOD0(distribute, void(void));
    MOCK_CONST_METHOD0(getProducts, std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>(void));
    MOCK_CONST_METHOD0(getSourceURL, std::string(void));

};


