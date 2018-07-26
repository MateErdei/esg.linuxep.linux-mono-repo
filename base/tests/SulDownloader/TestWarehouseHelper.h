///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once

#include "SulDownloader/IWarehouseRepository.h"
#include "SulDownloader/ConfigurationData.h"

#include <memory>
#include <functional>

namespace SulDownloader
{
    class TestWarehouseHelper
    {
    public:
        void replaceWarehouseCreator( std::function<std::unique_ptr<IWarehouseRepository>(const ConfigurationData&)> creator);
        void restoreWarehouseFactory();
    };

}



