///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "WarehouseRepositoryFactory.h"

#include "WarehouseRepository.h"

using namespace SulDownloader::suldownloaderdata;

namespace SulDownloader
{
    WarehouseRepositoryFactory& WarehouseRepositoryFactory::instance()
    {
        static WarehouseRepositoryFactory factory;
        return factory;
    }

    std::unique_ptr<IWarehouseRepository> WarehouseRepositoryFactory::fetchConnectedWarehouseRepository(
        const ConfigurationData& configurationData)
    {
        return m_creatorMethod(configurationData);
    }

    WarehouseRepositoryFactory::WarehouseRepositoryFactory() { restoreCreator(); }

    void WarehouseRepositoryFactory::replaceCreator(
        std::function<std::unique_ptr<IWarehouseRepository>(const ConfigurationData&)> creatorMethod)
    {
        m_creatorMethod = std::move(creatorMethod);
    }

    void WarehouseRepositoryFactory::restoreCreator()
    {
        m_creatorMethod = [](const ConfigurationData& configurationData) {
            auto wh = WarehouseRepository::FetchConnectedWarehouse(configurationData);
            return std::unique_ptr<IWarehouseRepository>(wh.release());
        };
    }

} // namespace SulDownloader