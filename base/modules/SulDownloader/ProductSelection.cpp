/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <cassert>
#include "ProductSelection.h"
#include "SulDownloaderException.h"
#include "Logger.h"

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
        StableSetIndex( int capacity ): m_trackIndexes(capacity,false), m_indexes()
        {

        }

        bool hasIndex( size_t index ) const
        {
            assert( index < m_trackIndexes.size());
            return m_trackIndexes[index];
        }

        void addIndex(size_t index)
        {
            if ( ! hasIndex(index))
            {
                m_trackIndexes[index ] = true;
                m_indexes.push_back(index);
            }
        }
        void addIndexes( const std::vector<size_t> & values)
        {
            for (size_t value: values)
            {
                addIndex(value);
            }
        }

        const std::vector<size_t>& values() const
        {
            return m_indexes;
        }
    };



}

namespace SulDownloader
{

    ProductSelector::ProductSelector(const std::string &productName, NamePrefix productNamePrefix, const std::string &releaseTag, const std::string &baseVersion)
    : m_productName( productName)
    , m_NamePrefix (productNamePrefix)
    , m_releaseTag(releaseTag)
    , m_baseVersion(baseVersion)

    {
        if ( m_productName.empty())
        {
            throw SulDownloaderException( "Cannot accept empty product name or prefix.");
        }
    }

    bool ProductSelector::keepProduct(const ProductMetadata & productInformation) const
    {
        size_t pos = productInformation.getLine().find(m_productName);
        if ( pos != 0)
        {
            // m_productname is not a prefix of productInformation.getLine()
            return false;
        }
        if (m_NamePrefix == NamePrefix::UseFullName && m_productName != productInformation.getLine())
        {
            return false;
        }

        if ( !productInformation.hasTag( m_releaseTag))
        {
            return false;
        }


        if (productInformation.getBaseVersion() == m_baseVersion)
        {
            return true;
        }

        return false;
    }

    std::string ProductSelector::targetProductName() const
    {
        return m_productName;
    }


    void ProductSelection::appendSelector(std::unique_ptr<ISingleProductSelector> productSelector)
    {
        m_selection.emplace_back(std::move(productSelector));
    }


    ProductSelection ProductSelection::CreateProductSelection(const ConfigurationData & configurationData)
    {
        ProductSelection productSelection;

        for(auto product : configurationData.getProductSelection())
        {
            ProductSelector::NamePrefix namePrefix;

            if(product.Prefix)
            {
                namePrefix = ProductSelector::UseNameAsPrefix;
            }
            else
            {
                namePrefix = ProductSelector::UseFullName;
            }

            productSelection.appendSelector( std::unique_ptr<ISingleProductSelector>( new ProductSelector(product.Name, namePrefix, product.releaseTag, product.baseVersion)));
        }

        return productSelection;
    }

    SelectedResultsIndexes ProductSelection::selectProducts(const std::vector<ProductMetadata> &warehouseProducts) const
    {
        SelectedResultsIndexes selection;
        StableSetIndex selectedProductsIndex(warehouseProducts.size());

        for ( auto & selector : m_selection)
        {
            auto selectedIndexes = selectedProducts(*selector, warehouseProducts);
            if ( selectedIndexes.empty())
            {
                selection.missing.push_back(selector->targetProductName());
            }
            else
            {
                selectedProductsIndex.addIndexes(selectedIndexes);
            }
        }

        selection.selected = selectedProductsIndex.values();

        for ( size_t i =0 ; i < warehouseProducts.size(); ++i)
        {
            if ( !selectedProductsIndex.hasIndex(i))
            {
                selection.notselected.push_back(i);
            }
        }

        return selection;
    }

    std::vector<size_t> ProductSelection::selectedProducts(const ISingleProductSelector & selector,
                                                        const std::vector<ProductMetadata> &warehouseProducts) const
    {
        StableSetIndex set(warehouseProducts.size());
        for ( size_t i = 0 ; i< warehouseProducts.size(); ++i)
        {
            if( selector.keepProduct(warehouseProducts[i]))
            {
                set.addIndex(i);
            }
        }
        return set.values();
    }
}