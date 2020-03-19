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

        [[nodiscard]] bool hasIndex(size_t index) const
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

        [[nodiscard]] const std::vector<size_t>& values() const { return m_indexes; }
    };

    class SubscriptionSelector : public virtual ISingleProductSelector
    {
    public:
        explicit SubscriptionSelector(ProductSubscription productSubscription);
        [[nodiscard]] std::string targetProductName() const override;

        [[nodiscard]] bool keepProduct(const SulDownloader::suldownloaderdata::ProductMetadata&) const override;

        [[nodiscard]] bool isProductRequired() const override;

    private:
        ProductSubscription m_productSubscription;
    };

    // Subscription Selector

    SubscriptionSelector::SubscriptionSelector(ProductSubscription productSubscription) :
            m_productSubscription(std::move(productSubscription))
    {
    }

    std::string SubscriptionSelector::targetProductName() const { return m_productSubscription.rigidName(); }

    bool SubscriptionSelector::keepProduct(const SulDownloader::suldownloaderdata::ProductMetadata& productInformation) const
    {
        // the selection is based on the following algorithm.

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

} // namespace

namespace SulDownloader
{
    // Product Selection

    ProductSelection ProductSelection::CreateProductSelection(const ConfigurationData& configurationData)
    {
        ProductSelection productSelection;

        auto& primary = configurationData.getPrimarySubscription();
        LOGSUPPORT("Product Selector: " << primary.toString() << ". Primary.");
        productSelection.appendSelector(std::make_unique<SubscriptionSelector>(primary));

        for (const auto& subscription : configurationData.getProductsSubscription())
        {
            LOGSUPPORT("Product Selector: " << subscription.toString());
            productSelection.appendSelector(std::make_unique<SubscriptionSelector>(subscription));
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
        LOGDEBUG("Selected "<< selectedProductsIndex.values().size() << " products total");

        StableSetIndex secondSelection(warehouseProducts.size());
        bool atLeastOneHasCore = false;
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
                    atLeastOneHasCore = true;
                }
            }
        }
        if (!atLeastOneHasCore && selection.missing.empty())
        {
            selection.missing.push_back(MissingCoreProduct());
        }
        selectedProductsIndex = secondSelection;
        LOGDEBUG("Filtered "<< selectedProductsIndex.values().size() << " products total");

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
                LOGDEBUG("Keeping "<<i);
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