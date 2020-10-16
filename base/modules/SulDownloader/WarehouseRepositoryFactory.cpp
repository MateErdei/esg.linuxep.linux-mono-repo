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

    std::unique_ptr<IWarehouseRepository> WarehouseRepositoryFactory::createWarehouseRepository()
    {
        return m_creatorMethod();
    }

    WarehouseRepositoryFactory::WarehouseRepositoryFactory() { restoreCreator(); }

    void WarehouseRepositoryFactory::replaceCreator(WarehouseRespositoryCreaterFunc creatorMethod)
    {
        m_creatorMethod = std::move(creatorMethod);
    }

    void WarehouseRepositoryFactory::restoreCreator()
    {
        m_creatorMethod = []() {
            return std::make_unique<WarehouseRepository>();
        };
    }

} // namespace SulDownloader