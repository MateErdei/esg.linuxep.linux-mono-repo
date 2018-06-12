//
// Created by pair on 06/06/18.
//

#include "ProductSelection.h"
#include "SulDownloaderException.h"
#include "Logger.h"

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

    bool ProductSelector::keepProduct(const ProductInformation & productInformation) const
    {
        auto pos = productInformation.getLine().find(m_productName);
        if ( pos != 0)
        {
            return false;
        }
        if (m_NamePrefix == NamePrefix::UseFullName && m_productName.compare(productInformation.getLine()) != 0)
        {
            return false;
        }

        if ( !productInformation.hasTag( m_releaseTag))
        {
            return false;
        }


        if (productInformation.getBaseVersion().compare(m_baseVersion) == 0)
        {
            return true;
        }

        return false;
    }

    std::string ProductSelector::targetProductName() const
    {
        return m_productName;
    }


    ProductSelection::ProductSelection()
    {

    }

    void ProductSelection::appendSelector(std::unique_ptr<ISingleProductSelector> productSelector)
    {
        m_selection.emplace_back(std::move(productSelector));
    }


    bool ProductSelection::keepProduct(const ProductInformation & productInformation) const
    {
        for( auto & selector : m_selection)
        {
            if( selector->keepProduct(productInformation))
            {
                return true;
            }
        }
        return false;
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

    // if a ISingleProductSelector does not return keepThis for any of the downloadedProducts, that selector has not been applied
    // and it refers to missing product.
    std::vector<std::string>
    ProductSelection::missingProduct(const std::vector<ProductInformation> &downloadedProducts) const
    {
        std::vector<std::string> missingProducts;
        for ( auto & selector : m_selection)
        {
            if ( !selectAtLeastOneProduct(*selector, downloadedProducts))
            {
                missingProducts.push_back(selector->targetProductName());
            }
        }

        return missingProducts;
    }

    bool ProductSelection::selectAtLeastOneProduct(ISingleProductSelector &selector,
                                                   const std::vector<ProductInformation> &downloadedProducts) const
    {
        for( auto & productInfo : downloadedProducts)
        {
            if ( selector.keepProduct(productInfo))
            {
                return true;
            }
        }
        return false;
    }
}