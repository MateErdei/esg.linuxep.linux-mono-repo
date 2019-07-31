/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ConfigurationData.h"
#include "ProductMetadata.h"

#include <Common/UtilityImpl/VectorAsSet.h>

#include <memory>
#include <vector>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class ISingleProductSelector
        {
        public:
            virtual bool keepProduct(const suldownloaderdata::ProductMetadata&) const = 0;

            virtual bool isProductRequired() const = 0;

            virtual std::string targetProductName() const = 0;

            virtual ~ISingleProductSelector() = default;
        };

        class SubscriptionSelector : public virtual ISingleProductSelector
        {
        public:
            SubscriptionSelector(const ProductSubscription& productSubscription);
            std::string targetProductName() const override;

            bool keepProduct(const suldownloaderdata::ProductMetadata&) const override;

            bool isProductRequired() const override;

        private:
            ProductSubscription m_productSubscription;
        };

        struct SelectedResultsIndexes
        {
            std::vector<size_t> selected;
            std::vector<size_t> notselected;
            std::vector<std::string> missing;
        };

        /**
         * ProductSelection is responsible to define the products that must be downloaded from the WarehouseRepository
         * as well as report if any required product can not be found in the remote warehouse repository.
         */

        class ProductSelection
        {
            ProductSelection() = default;

        public:
            static std::string MissingCoreProduct();
            using ProductMetaDataVector = std::vector<suldownloaderdata::ProductMetadata>;
            using ISingleProductSelectorPtr = std::unique_ptr<ISingleProductSelector>;

            static ProductSelection CreateProductSelection(const suldownloaderdata::ConfigurationData&);

            void appendSelector(ISingleProductSelectorPtr selector);

            SelectedResultsIndexes selectProducts(const ProductMetaDataVector& warehouseProducts) const;

        private:
            std::vector<ISingleProductSelectorPtr> m_selection;
            Common::UtilityImpl::VectorAsSet m_features;
            bool passFeatureSetSelection(const ProductMetadata&) const;
            std::vector<size_t> selectedProducts(
                const ISingleProductSelector&,
                const ProductMetaDataVector& warehouseProducts) const;
        };

    } // namespace suldownloaderdata
} // namespace SulDownloader
