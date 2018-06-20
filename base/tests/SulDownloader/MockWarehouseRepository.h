///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_MOCKWAREHOUSEREPOSITORY_H
#define EVEREST_BASE_MOCKWAREHOUSEREPOSITORY_H

#include "SulDownloader/IWarehouseRepository.h"
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

#endif //EVEREST_BASE_MOCKWAREHOUSEREPOSITORY_H
