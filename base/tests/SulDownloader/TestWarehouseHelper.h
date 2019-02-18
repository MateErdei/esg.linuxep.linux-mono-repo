///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/IWarehouseRepository.h>

#include <functional>
#include <memory>

namespace SulDownloader
{
    class TestWarehouseHelper
    {
    public:
        void replaceWarehouseCreator(
            std::function<suldownloaderdata::IWarehouseRepositoryPtr(const suldownloaderdata::ConfigurationData&)>
                creator);
        void restoreWarehouseFactory();
    };

} // namespace SulDownloader
