/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProductSelection.h"

#include "Logger.h"
#include "SulDownloaderException.h"
#include <list>
#include <cassert>
#include <Common/UtilityImpl/StringUtils.h>

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
        [[nodiscard]] bool empty() const { return m_indexes.empty(); }
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


    struct MatchLineVersion
    {
        std::string m_line; 
        std::string m_version; 
        MatchLineVersion(std::string line, std::string version): m_line(line), m_version(version)
        {
        }
        bool operator()(const SulDownloader::suldownloaderdata::ProductMetadata& candidate) const{
            return candidate.getLine() == m_line && candidate.getVersion() == m_version; 
        }        
    };

    bool shouldExpandComponentSuiteToProducts(const ProductMetadata& productSubscriptionMetaData)
    {
        if ( productSubscriptionMetaData.subProducts().empty())
        {
            
            return false; 
        }
        else
        {
            if (productSubscriptionMetaData.getFeatures().empty())
            {
                return true; 
            }
            else
            {
                // this should be false: do not expand the component suite as there are features associated with the component suite. 
                // but a special case has to be made to ServerProtectionLinux-Base to handle transition where the old-version of 
                // suldownloader needs to be used for upgrading (hence, features will be associated with that rigidname for some time.)
                return productSubscriptionMetaData.getLine() == "ServerProtectionLinux-Base"; 
            }                        
        }
    }

    StableSetIndex mapSubscriptionToProducts( ProductMetadata productSubscriptionMetaData, const std::vector<ProductMetadata>& warehouseProducts )
    {
        StableSetIndex set(warehouseProducts.size());

        for( auto & subComp: productSubscriptionMetaData.subProducts())
        {
            auto pos = std::find_if(warehouseProducts.begin(), warehouseProducts.end(), MatchLineVersion(subComp.m_line, subComp.m_version)); 
            if ( pos != warehouseProducts.end())
            {
                set.addIndex(std::distance(warehouseProducts.begin(), pos)); 
            }
        }
        return set; 
    }

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
        StableSetIndex selectedSubscriptionsIndex(warehouseProducts.size());

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
                selectedSubscriptionsIndex.addIndexes(selectedIndexes);
            }
        }
        
        LOGDEBUG("Selected "<< selectedSubscriptionsIndex.values().size() << " subscriptions total");
        for( auto & index : selectedSubscriptionsIndex.values())
        {
             const auto& warehouseProduct = warehouseProducts[index];
             LOGDEBUG("Selected Subscription: " << warehouseProduct.getName()); 
        }

        StableSetIndex selectedProductsIndex(warehouseProducts.size());
        for( auto & index : selectedSubscriptionsIndex.values())
        {
            if (shouldExpandComponentSuiteToProducts(warehouseProducts[index]))
            {
                const auto& subscriptionProduct = warehouseProducts[index];
                auto selectedIndexes = mapSubscriptionToProducts(warehouseProducts[index] , warehouseProducts );

                // in order to handle the 'strange case' that the order of the component could not be the order 
                // defined in the warehouse configuration, it has been decided, that if it were to be a product that needs to be installed
                // first inside a component suite, that product will have the name of the component suite plus a suffix. 
                // the lines below, garantee that a component within a component suite whose rigidline contains the component suite 
                // will be placed in the front of the 'list'. 
                // if the order were garantted, it woudl be possible to do: 
                //  selectedProductsIndex.addIndexes(selectedIndexes);

                std::list<size_t> indexes; 
                for( auto sub_index: selectedIndexes.values())
                {
                    const auto & product = warehouseProducts[sub_index]; 
                    if ( Common::UtilityImpl::StringUtils::startswith(product.getLine(), subscriptionProduct.getLine()))
                    {
                        indexes.push_front(sub_index); 
                    }else
                    {
                        indexes.push_back(sub_index); 
                    }                
                } 
                for( auto sub_index: indexes)
                {
                    selectedProductsIndex.addIndex(sub_index); 
                }
            }
            else
            {
                selectedProductsIndex.addIndex(index); 
            }            
        }

        for( auto & index : selectedProductsIndex.values())
        {
             const auto& warehouseProduct = warehouseProducts[index];
             LOGDEBUG("Selected Product Indexes after Map Subscription to Product: " << warehouseProduct.getName()); 
        }


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
        
        std::vector<ProductMetadata> selectedProductsView; 
        for( auto & index : selectedProductsIndex.values())
        {
             const auto& warehouseProduct = warehouseProducts[index];
             LOGDEBUG("Selected product: " << warehouseProduct.getName()); 
             selectedProductsView.push_back(warehouseProduct); 
        }

        StableSetIndex finalSubscriptionSelection(warehouseProducts.size());
        for( auto & index: selectedSubscriptionsIndex.values())
        {
            if ( shouldExpandComponentSuiteToProducts(warehouseProducts[index]))
            {
                 auto selectedIndexes = mapSubscriptionToProducts(warehouseProducts[index] , selectedProductsView );
                 // it he selection returns empty it means that all the products under the component suite were removed, hence, it should not be added.
                 if (!selectedIndexes.empty())
                 {
                    finalSubscriptionSelection.addIndex(index); 
                 }
            }
            else
            {
                finalSubscriptionSelection.addIndex(index); 
            }
        }

        selection.selected = selectedProductsIndex.values();
        selection.selected_subscriptions = finalSubscriptionSelection.values();

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
                for( auto & product: warehouseProducts[i].subProducts())
                {
                    LOGDEBUG("Selected component has line,version="<< product.m_line << ", "<< product.m_version); 
                }
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