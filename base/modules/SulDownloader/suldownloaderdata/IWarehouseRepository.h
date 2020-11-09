/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ProductMetadata.h"

#include <bits/unique_ptr.h>

#include <string>
#include <vector>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class ConfigurationData;
        class ConnectionSetup;
        class DownloadedProduct;
        class ProductSelection;
        struct WarehouseError;
        struct ProductInfo
        {
            std::string m_rigidName;
            std::string m_productName;
            std::string m_version;
        };

        struct SubscriptionInfo
        {
            std::string rigidName;
            std::string version;
            SubProducts subProducts;
        };

        /**
         * Interface for WarehouseRepository to enable tests.
         * For documentation, see WarehouseRepository.h
         */
        class IWarehouseRepository
        {
        public:
            using SulLogsVector = std::vector<std::string>;

            virtual ~IWarehouseRepository() = default;

            /**
             * Used to check if the WarehouseRepository reported an error
             * @return true, if WarehouseRepository has error, false otherwise
             */
            [[nodiscard]] virtual bool hasError() const = 0;

            /**
             * Do we have an error that means we should abort the update immediately?
             *
             * Currently: PACKAGESOURCEMISSING
             *
             * Subset of hasError()
             *
             * @return true if we should abort the update
             */
            [[nodiscard]] virtual bool hasImmediateFailError() const = 0;

            [[nodiscard]] virtual WarehouseError getError() const = 0;

            virtual void synchronize(ProductSelection&) = 0;

            virtual void distribute() = 0;

            [[nodiscard]] virtual std::vector<suldownloaderdata::DownloadedProduct> getProducts() const = 0;

            [[nodiscard]] virtual std::string getSourceURL() const = 0;

            [[nodiscard]] virtual std::string getProductDistributionPath(const suldownloaderdata::DownloadedProduct&) const = 0;

            [[nodiscard]] virtual std::vector<ProductInfo> listInstalledProducts() const = 0;
            [[nodiscard]] virtual std::vector<suldownloaderdata::SubscriptionInfo> listInstalledSubscriptions() const = 0;


            /**
             * Attempt to connect to a provided connection setup information.
             *
             *
             * @param connectionSetup
             * @param supplementOnly  Only download supplements
             * @param configurationData
             * @return
             */
            virtual bool tryConnect(
                const suldownloaderdata::ConnectionSetup& connectionSetup,
                bool supplementOnly,
                const suldownloaderdata::ConfigurationData& configurationData) = 0;

            /**
             * Get the logs for this SUL connection
             * @return
             */
            virtual SulLogsVector getLogs() const = 0;

            virtual void dumpLogs() const = 0;

            virtual void reset() = 0;

        };

        using IWarehouseRepositoryPtr = std::unique_ptr<IWarehouseRepository>;

    } // namespace suldownloaderdata
} // namespace SulDownloader
