///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once


#include <vector>
#include <string>
#include <bits/unique_ptr.h>

namespace SulDownloader
{
    class ProductSelection;
    struct WarehouseError;

    namespace suldownloaderdata
    {
        class DownloadedProduct;

        /**
         * Interface for WarehouseRepository to enable tests.
         * For documentation, see WarehouseRepository.h
         */
        class IWarehouseRepository
        {
        public:
            virtual ~IWarehouseRepository() = default;

            virtual bool hasError() const = 0;

            virtual WarehouseError getError() const = 0;

            virtual void synchronize(ProductSelection& productSelection) = 0;

            virtual void distribute() = 0;

            virtual std::vector<suldownloaderdata::DownloadedProduct> getProducts() const = 0;

            virtual std::string getSourceURL() const = 0;

        };

        using IWarehouseRepositoryPtr = std::unique_ptr<IWarehouseRepository>;


    }
}


