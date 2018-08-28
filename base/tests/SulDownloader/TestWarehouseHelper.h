///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once

#include <SulDownloader/suldownloaderdata/IWarehouseRepository.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>

#include <memory>
#include <functional>

namespace SulDownloader
{
    class TestWarehouseHelper
    {
    public:
        void replaceWarehouseCreator( std::function<std::unique_ptr<IWarehouseRepository>(const suldownloaderdata::ConfigurationData&)> creator);
        void restoreWarehouseFactory();
    };

}



