// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once
#include "ProductMetadata.h"

#include "Common/Policy/UpdateSettings.h"

#include <memory>
namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class ConnectionSetup;
        class DownloadedProduct;
        class ProductSelection;
        struct RepositoryError;
        struct ProductInfo
        {
            std::string m_rigidName;
            std::string m_productName;
            std::string m_version;
            std::string m_installedVersion;

            [[nodiscard]] bool operator==(const ProductInfo& other) const
            {
                // clang-format off
                return m_rigidName == other.m_rigidName &&
                       m_productName == other.m_productName &&
                       m_version == other.m_version &&
                       m_installedVersion == other.m_installedVersion;
                // clang-format on
            }
        };

        struct SubscriptionInfo
        {
            std::string rigidName;
            std::string version;
            SubProducts subProducts;
        };

        class IRepository
        {
        public:
            virtual ~IRepository() = default;
            /**
             * Used to check if the Repository reported an error
             * @return true, if Repository has error, false otherwise
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

            [[nodiscard]] virtual RepositoryError getError() const = 0;

            [[nodiscard]] virtual std::vector<suldownloaderdata::DownloadedProduct> getProducts() const = 0;

            [[nodiscard]] virtual std::string getSourceURL() const = 0;

            [[nodiscard]] virtual std::vector<suldownloaderdata::SubscriptionInfo> listInstalledSubscriptions() const = 0;

            [[nodiscard]] virtual std::string getProductDistributionPath(const suldownloaderdata::DownloadedProduct&) const = 0;

            [[nodiscard]] virtual std::vector<ProductInfo> listInstalledProducts() const = 0;

            virtual void purge() const = 0;

        };

        using IRepositoryPtr = std::unique_ptr<IRepository>;
    }
}
