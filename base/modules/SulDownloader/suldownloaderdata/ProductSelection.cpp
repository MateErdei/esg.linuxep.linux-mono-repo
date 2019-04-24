/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

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
    // Product Selector

    ProductSelector::ProductSelector(
        const std::string& productName,
        NamePrefix productNamePrefix,
        const std::string& releaseTag,
        const std::string& baseVersion) :
        m_productName(productName),
        m_NamePrefix(productNamePrefix),
        m_releaseTag(releaseTag),
        m_baseVersion(baseVersion)

    {
        if (m_productName.empty())
        {
            throw SulDownloaderException("Cannot accept empty product name or prefix.");
        }
    }

    bool ProductSelector::keepProduct(const ProductMetadata& productInformation) const
    {
        size_t pos = productInformation.getLine().find(m_productName);
        if (pos != 0)
        {
            // m_productname is not a prefix of productInformation.getLine()
            return false;
        }
        if (m_NamePrefix == NamePrefix::UseFullName && m_productName != productInformation.getLine())
        {
            return false;
        }

        if (!productInformation.hasTag(m_releaseTag))
        {
            return false;
        }

        if (productInformation.getBaseVersion() == m_baseVersion || m_releaseTag == "RECOMMENDED")
        {
            return true;
        }

        return false;
    }

    std::string ProductSelector::targetProductName() const { return m_productName; }

    bool ProductSelector::isProductRequired() const { return (m_NamePrefix != NamePrefix::UseNameAsPrefix); }

    void ProductSelection::appendSelector(std::unique_ptr<ISingleProductSelector> productSelector)
    {
        m_selection.emplace_back(std::move(productSelector));
    }


    // Subscription Selector

    SubscriptionSelector::SubscriptionSelector(const ProductSubscription& productSubscription)
                                               : m_productSubscription(productSubscription)
    {

    }

    std::string SubscriptionSelector::targetProductName() const
    {
        return m_productSubscription.rigidName();
    }

    bool SubscriptionSelector::keepProduct(const suldownloaderdata::ProductMetadata& productInformation) const
    {

        // the selection is based on the following algorith.

        // It has to match rigid name.
        // If the subscription has fixed version, it has precedence to tags and the fixversion must match.
        // It the subscription has empty fix version, the tag system is used.
        // The tag selection is the following:
        //   the product must have the tag in the subscription.
        //   and if the tag is recommended, it may ignore the base version.

        if ( productInformation.getLine() != m_productSubscription.rigidName() )
        {
            return false;
        }

        if ( !m_productSubscription.fixVersion().empty())
        {
            return productInformation.getVersion() == m_productSubscription.fixVersion();
        }

        if (!productInformation.hasTag(m_productSubscription.tag()))
        {
            return false;
        }

        return productInformation.getBaseVersion() == m_productSubscription.baseVersion() ||
        m_productSubscription.tag() == "RECOMMENDED";

    }

    bool SubscriptionSelector::isProductRequired() const
    {
        return true;
    }


    // Product Selection


    ProductSelection ProductSelection::CreateProductSelection(const ConfigurationData& configurationData)
    {
        ProductSelection productSelection;

        auto & primary = configurationData.getPrimarySubscription();
        LOGSUPPORT("Product Selector: " << primary.toString() << ". Primary.");
        productSelection.appendSelector(std::unique_ptr<ISingleProductSelector>(
                new SubscriptionSelector(primary)));

        for ( auto subscription : configurationData.getProductsSubscription())
        {
            LOGSUPPORT("Product Selector: " << subscription.toString());
            productSelection.appendSelector(std::unique_ptr<ISingleProductSelector>(
                    new SubscriptionSelector(subscription)));
        }

        productSelection.m_features.setEntries(configurationData.getFeatures() );

        return productSelection;
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

        if ( m_useFeatures )
        {
            StableSetIndex secondSelection(warehouseProducts.size());
            bool atLeastOnHasCore = false;
            auto hasCore = [](const std::vector<std::string>& features) {
                return std::find(features.begin(), features.end(), "CORE") != features.end();
            };

            for (auto index: selectedProductsIndex.values())
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
        }

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
        if ( !m_useFeatures)
        {
            return true;
        }

        for( const auto & feature : productMetadata.getFeatures())
        {
            if( m_features.hasEntry(feature))
            {
                return true;
            }
        }

        return false;
    }

    std::string ProductSelection::MissingCoreProduct()
    {
        return "Missing CORE product";
    }


} // namespace SulDownloader