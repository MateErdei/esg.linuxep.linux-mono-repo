/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
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
        struct ProductInfo
        {
            std::string m_rigidName;
            std::string m_productName;
            std::string m_version;
        };

        struct SubscriptionInfo{
            std::string rigidName; 
            std::string version; 
        };
        bool operator==(const SubscriptionInfo & , const SubscriptionInfo &);

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

            virtual void synchronize(ProductSelection&) = 0;

            virtual void distribute() = 0;

            virtual std::vector<suldownloaderdata::DownloadedProduct> getProducts() const = 0;

            virtual std::string getSourceURL() const = 0;

            virtual std::string getProductDistributionPath(const suldownloaderdata::DownloadedProduct&) const = 0;

            virtual std::vector<ProductInfo> listInstalledProducts() const = 0;
            virtual std::vector<suldownloaderdata::SubscriptionInfo> listInstalledSubscriptions() const = 0; 
        };

        using IWarehouseRepositoryPtr = std::unique_ptr<IWarehouseRepository>;

    } // namespace suldownloaderdata
} // namespace SulDownloader
