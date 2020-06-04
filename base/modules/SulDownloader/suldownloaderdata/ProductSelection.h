/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

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

        struct SelectedResultsIndexes
        {
            // product selected to be installed
            std::vector<size_t> selected;
            // refers to the subscriptions that were targeted in the subscriptions part of the ALC policy.
            std::vector<size_t> selected_subscriptions;
            // these are the products that will be instructed not to download from the wharehouse catalogue
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
