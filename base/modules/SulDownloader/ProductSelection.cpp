//
// Created by pair on 06/06/18.
//

#include "ProductSelection.h"
#include "SulDownloaderException.h"
namespace SulDownloader
{

    RecommendedProductSelector::RecommendedProductSelector(const std::string &productName, NamePrefix productNamePrefix)
    : m_productName( productName)
    , m_NamePrefix (productNamePrefix)

    {
        if ( m_productName.empty())
        {
            throw SulDownloaderException( "Cannot accept empty product name or prefix.");
        }
    }

    bool RecommendedProductSelector::keepProduct(const ProductInformation & productInformation) const
    {
        auto pos = productInformation.getName().find(m_productName);
        if ( pos != 0)
        {
            return false;
        }
        if (m_NamePrefix == NamePrefix::UseFullName && m_productName.compare(productInformation.getName()) != 0)
        {
            return false;
        }

        if ( productInformation.hasRecommended())
        {
            return true;
        }

        return false;
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
            RecommendedProductSelector::NamePrefix namePrefix;

            if(product.Prefix)
            {
                namePrefix = RecommendedProductSelector::UseNameAsPrefix;
            }
            else
            {
                namePrefix = RecommendedProductSelector::UseFullName;
            }

            productSelection.appendSelector( std::unique_ptr<ISingleProductSelector>( new RecommendedProductSelector(product.Name, namePrefix)));
        }

        return productSelection;
    }
}