///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_TESTWAREHOUSEHELPER_H
#define EVEREST_BASE_TESTWAREHOUSEHELPER_H
#include "SulDownloader/IWarehouseRepository.h"
#include "SulDownloader/ConfigurationData.h"

#include <memory>


namespace SulDownloader
{
    class TestWarehouseHelper
    {
    public:
        void replaceWarehouseCreator( std::function<std::unique_ptr<IWarehouseRepository>(const ConfigurationData&)> creator);
        void restoreWarehouseFactory();
    };

}


#endif //EVEREST_BASE_TESTWAREHOUSEHELPER_H
