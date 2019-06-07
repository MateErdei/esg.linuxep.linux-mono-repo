/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CatalogueInfo.h"
#include "DownloadedProduct.h"
#include "ProductMetadata.h"
namespace  SulDownloader
{
    namespace suldownloaderdata
    {

        void CatalogueInfo::addInfo(const std::string & line, const std::string & version, const std::string& productName)
        {
            m_catalogue[Key{line, version}] = productName;
        }

        std::string CatalogueInfo::productName(const std::string& productLine, const std::string& productVersion) const
        {
            auto found = m_catalogue.find( Key{productLine,productVersion} );
            if ( found == m_catalogue.end())
            {
                return std::string{};
            }
            return found->second;
        }

        std::vector<suldownloaderdata::ProductInfo>
        CatalogueInfo::calculatedListProducts(const std::vector<suldownloaderdata::DownloadedProduct>& downloadedProducts,
                                              const suldownloaderdata::CatalogueInfo& catalogueInfo)
        {
            std::vector<ProductMetadata> productsMetadata;
            for( auto & downloadedProduct : downloadedProducts)
            {
                productsMetadata.push_back(downloadedProduct.getProductMetadata());
            }
            SubProducts subProducts =  ProductMetadata::combineSubProducts( productsMetadata);
            std::vector<suldownloaderdata::ProductInfo> productsInfo;
            for( auto & subProduct : subProducts)
            {
                std::string productName = catalogueInfo.productName(subProduct.m_line, subProduct.m_version);
                ProductInfo productInfo;
                productInfo.m_rigidName = subProduct.m_line;
                productInfo.m_version = subProduct.m_version;
                productInfo.m_productName = productName;
                productsInfo.push_back( productInfo  );
            }
            return productsInfo;
        }
    }
}