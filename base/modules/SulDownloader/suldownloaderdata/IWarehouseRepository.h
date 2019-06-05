///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once

#include <bits/unique_ptr.h>

#include <string>
#include <vector>

namespace SulDownloader
{
    namespace suldownloaderdata
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

            virtual WarehouseError getError() const = 0;

            virtual void synchronize(ProductSelection& productSelection) = 0;

            virtual void distribute() = 0;

            virtual std::vector<suldownloaderdata::DownloadedProduct> getProducts() const = 0;

            virtual std::string getSourceURL() const = 0;

            virtual std::string getProductDistributionPath(const suldownloaderdata::DownloadedProduct&) const = 0;
        };

        using IWarehouseRepositoryPtr = std::unique_ptr<IWarehouseRepository>;

    } // namespace suldownloaderdata
} // namespace SulDownloader
