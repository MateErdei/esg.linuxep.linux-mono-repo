///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "TestWarehouseHelper.h"
#include "SulDownloader/WarehouseRepositoryFactory.h"

void SulDownloader::TestWarehouseHelper::replaceWarehouseCreator(
        std::function<std::unique_ptr<SulDownloader::IWarehouseRepository>(
                const SulDownloader::ConfigurationData &)> creator)
{
    SulDownloader::WarehouseRepositoryFactory::instance().replaceCreator(creator);
}
