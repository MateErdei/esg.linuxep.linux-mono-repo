///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "TestWarehouseHelper.h"
#include "SulDownloader/WarehouseRepositoryFactory.h"

void SulDownloader::TestWarehouseHelper::replaceWarehouseCreator(
        std::function<suldownloaderdata::IWarehouseRepositoryPtr(
                const SulDownloader::suldownloaderdata::ConfigurationData &)> creator)
{
    SulDownloader::WarehouseRepositoryFactory::instance().replaceCreator(std::move(creator));
}

void SulDownloader::TestWarehouseHelper::restoreWarehouseFactory()
{   SulDownloader::WarehouseRepositoryFactory::instance().restoreCreator();

}
