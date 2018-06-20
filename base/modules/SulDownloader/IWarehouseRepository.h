///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_IWAREHOUSEREPOSITORY_H
#define EVEREST_BASE_IWAREHOUSEREPOSITORY_H

#include <vector>

namespace SulDownloader
{
    class DownloadedProduct;
    class ProductSelection;
    struct WarehouseError;
    /**
     * Interface for WarehouseRepository to enable tests.
     * For documentation, see WarehouseRepository.h
     */
    class IWarehouseRepository
    {
    public:
        virtual ~IWarehouseRepository() = default;

        virtual bool hasError() const = 0;
        virtual WarehouseError getError() const =0;

        virtual void synchronize( ProductSelection & productSelection) =0 ;

        virtual void distribute() = 0;

        virtual std::vector<DownloadedProduct> getProducts() const = 0;

    };


}

#endif //EVEREST_BASE_IWAREHOUSEREPOSITORY_H
