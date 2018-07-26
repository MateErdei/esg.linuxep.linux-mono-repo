/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "SulDownloader/IWarehouseRepository.h"
#include "SulDownloader/WarehouseError.h"
#include "SulDownloader/DownloadedProduct.h"
#include "SulDownloader/ProductSelection.h"
using namespace ::testing;
using namespace SulDownloader;

class MockWarehouseRepository: public SulDownloader::IWarehouseRepository
{
public:
    MOCK_CONST_METHOD0(hasError, bool(void));
    MOCK_CONST_METHOD0(getError, WarehouseError(void));
    MOCK_METHOD1(synchronize, void(SulDownloader::ProductSelection&));
    MOCK_METHOD0(distribute, void(void));
    MOCK_CONST_METHOD0(getProducts, std::vector<SulDownloader::DownloadedProduct>(void));

};


