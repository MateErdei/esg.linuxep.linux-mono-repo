/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProductSelection.h"

#include "Logger.h"
#include "SulDownloaderException.h"

#include <cassert>

using namespace SulDownloader::suldownloaderdata;

namespace
{
    /**
     * Stable: preserve the order that index are added
     * Set: value guarantees no duplication
     * Index: Meant for index of arrays hence valid values are 0->Capacity-1
     */
    class StableSetIndex
    {
        std::vector<bool> m_trackIndexes;
        std::vector<size_t> m_indexes;

    public:
        explicit StableSetIndex(size_t capacity) : m_trackIndexes(capacity, false), m_indexes() {}

        bool hasIndex(size_t index) const
        {
            assert(index < m_trackIndexes.size());
            return m_trackIndexes[index];
        }

        void addIndex(size_t index)
        {
            if (!hasIndex(index))
            {
                m_trackIndexes[index] = true;
                m_indexes.push_back(index);
            }
        }
        void addIndexes(const std::vector<size_t>& values)
        {
            for (size_t value : values)
            {
                addIndex(value);
            }
        }

        const std::vector<size_t>& values() const { return m_indexes; }
    };

} // namespace

namespace SulDownloader
{
    // Subscription Selector

    SubscriptionSelector::SubscriptionSelector(const ProductSubscription& productSubscription) :
        m_productSubscription(productSubscription)
    {
    }

    std::string SubscriptionSelector::targetProductName() const { return m_productSubscription.rigidName(); }

    bool SubscriptionSelector::keepProduct(const suldownloaderdata::ProductMetadata& productInformation) const
    {
        // the selection is based on the following algorith.

        // It has to match rigid name.
        // If the subscription has fixed version, it has precedence to tags and the fixedversion must match.
        // It the subscription has empty fix version, the tag system is used.
        // The tag selection is the following:
        //   the product must have the tag in the subscription.
        //   and if the tag is recommended, it may ignore the base version.

        if (productInformation.getLine() != m_productSubscription.rigidName())
        {
            return false;
        }

        if (!m_productSubscription.fixedVersion().empty())
        {
            return productInformation.getVersion() == m_productSubscription.fixedVersion();
        }

        if (!productInformation.hasTag(m_productSubscription.tag()))
        {
            return false;
        }

        return productInformation.getBaseVersion() == m_productSubscription.baseVersion() ||
               m_productSubscription.tag() == "RECOMMENDED";
    }

    bool SubscriptionSelector::isProductRequired() const { return true; }

    // Product Selection

    ProductSelection ProductSelection::CreateProductSelection(const ConfigurationData& configurationData)
    {
        ProductSelection productSelection;

        auto& primary = configurationData.getPrimarySubscription();
        LOGSUPPORT("Product Selector: " << primary.toString() << ". Primary.");
        productSelection.appendSelector(std::unique_ptr<ISingleProductSelector>(new SubscriptionSelector(primary)));

        for (auto subscription : configurationData.getProductsSubscription())
        {
            LOGSUPPORT("Product Selector: " << subscription.toString());
            productSelection.appendSelector(
                std::unique_ptr<ISingleProductSelector>(new SubscriptionSelector(subscription)));
        }

        productSelection.m_features.setEntries(configurationData.getFeatures());

        return productSelection;
    }

    void ProductSelection::appendSelector(std::unique_ptr<ISingleProductSelector> productSelector)
    {
        m_selection.emplace_back(std::move(productSelector));
    }

    SelectedResultsIndexes ProductSelection::selectProducts(const std::vector<ProductMetadata>& warehouseProducts) const
    {
        SelectedResultsIndexes selection;
        StableSetIndex selectedProductsIndex(warehouseProducts.size());

        for (auto& selector : m_selection)
        {
            auto selectedIndexes = selectedProducts(*selector, warehouseProducts);

            // if product is not required do not force download failure.
            if (selectedIndexes.empty() && selector->isProductRequired())
            {
                selection.missing.push_back(selector->targetProductName());
            }
            else
            {
                selectedProductsIndex.addIndexes(selectedIndexes);
            }
        }

        StableSetIndex secondSelection(warehouseProducts.size());
        bool atLeastOnHasCore = false;
        auto hasCore = [](const std::vector<std::string>& features) {
            return std::find(features.begin(), features.end(), "CORE") != features.end();
        };

        for (auto index : selectedProductsIndex.values())
        {
            const auto& warehouseProduct = warehouseProducts[index];
            if (passFeatureSetSelection(warehouseProduct))
            {
                secondSelection.addIndex(index);
                if (hasCore(warehouseProduct.getFeatures()))
                {
                    atLeastOnHasCore = true;
                }
            }
        }
        if (!atLeastOnHasCore && selection.missing.empty())
        {
            selection.missing.push_back(MissingCoreProduct());
        }
        selectedProductsIndex = secondSelection;

        selection.selected = selectedProductsIndex.values();

        for (size_t i = 0; i < warehouseProducts.size(); ++i)
        {
            if (!selectedProductsIndex.hasIndex(i))
            {
                selection.notselected.push_back(i);
            }
        }

        return selection;
    }

    std::vector<size_t> ProductSelection::selectedProducts(
        const ISingleProductSelector& selector,
        const std::vector<ProductMetadata>& warehouseProducts) const
    {
        StableSetIndex set(warehouseProducts.size());
        for (size_t i = 0; i < warehouseProducts.size(); ++i)
        {
            if (selector.keepProduct(warehouseProducts[i]))
            {
                set.addIndex(i);
            }
        }
        return set.values();
    }

    bool ProductSelection::passFeatureSetSelection(const ProductMetadata& productMetadata) const
    {
        for (const auto& feature : productMetadata.getFeatures())
        {
            if (m_features.hasEntry(feature))
            {
                return true;
            }
        }

        return false;
    }

    std::string ProductSelection::MissingCoreProduct() { return "Missing CORE product"; }

} // namespace SulDownloader