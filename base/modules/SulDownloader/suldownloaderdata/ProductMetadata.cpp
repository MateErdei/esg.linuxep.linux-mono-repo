/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ProductMetadata.h"
#include "Logger.h"
#include <Common/UtilityImpl/OrderedSet.h>
#include <cassert>
#include <stdexcept>

namespace std
{
    template<> struct hash<SulDownloader::suldownloaderdata::ProductKey>
    {
        typedef  SulDownloader::suldownloaderdata::ProductKey argument_type;
        typedef std::size_t result_type;
        result_type operator()(const argument_type& productKey) const noexcept
        {
            result_type const h1 ( std::hash<std::string>{}(productKey.m_line) );
            result_type const h2 ( std::hash<std::string>{}(productKey.m_version) );
            return h1 ^ h2;
        }
    };
}

using namespace SulDownloader::suldownloaderdata;

const std::string& ProductMetadata::getLine() const
{
    return m_line;
}

void ProductMetadata::setLine(const std::string& line)
{
    m_line = line;
}

const std::string& ProductMetadata::getName() const
{
    return m_name;
}

void ProductMetadata::setName(const std::string& name)
{
    m_name = name;
}

void ProductMetadata::setTags(const std::vector<Tag>& tags)
{
    m_tags = tags;
}

bool ProductMetadata::hasTag(const std::string& releaseTag) const
{
    for (auto& tag : m_tags)
    {
        if (tag.tag == releaseTag)
        {
            return true;
        }
    }
    return false;
}

void ProductMetadata::setVersion(const std::string& version)
{
    m_version = version;
}

std::string ProductMetadata::getBaseVersion() const
{
    return m_baseVersion;
}

const std::string& ProductMetadata::getVersion() const
{
    return m_version;
}

void ProductMetadata::setDefaultHomePath(const std::string& defaultHomeFolder)
{
    m_defaultHomeFolder = defaultHomeFolder;
}

const std::vector<Tag> ProductMetadata::tags() const
{
    return m_tags;
}

void ProductMetadata::setBaseVersion(const std::string& baseVersion)
{
    m_baseVersion = baseVersion;
}

void ProductMetadata::setFeatures(const std::vector<std::string>& features)
{
    m_features = features;
}

const std::vector<std::string>& ProductMetadata::getFeatures() const
{
    return  m_features;
}

SubProducts ProductMetadata::extractSubProductsFromSulSubComponents(const std::vector<std::string>& sulSubComponents)
{
    SulDownloader::suldownloaderdata::SubProducts subProducts;
    for( auto & sulSubComponent: sulSubComponents)
    {
        try
        {
            subProducts.push_back( extractProductKeyFromSubComponent(sulSubComponent) );
        }
        catch ( std::invalid_argument & )
        {
            LOGWARN( "SubComponent received from Sul is not as expected. Received: " << sulSubComponent);
        }

    }
    return subProducts;
}

void ProductMetadata::setSubProduts(const SubProducts& subProducts)
{
    m_subProducts = subProducts;
}

const SubProducts& ProductMetadata::subProducts() const
{
    return m_subProducts;
}

ProductKey ProductMetadata::extractProductKeyFromSubComponent(const std::string& sulSubComponent)
{
    std::string::size_type pos =  sulSubComponent.rfind(";");
    if( pos == std::string::npos)
    {
        throw std::invalid_argument( "Missing ;");
    }
    if( sulSubComponent == ";")
    {
        throw std::invalid_argument("Empty rigidname is not valid");
    }
    std::string rigidName = sulSubComponent.substr(0,pos);
    std::string version = sulSubComponent.substr(pos+1);

    return   {rigidName, version};
}

// This method is meant to extract the components or the first level subcomponents that will eventually
// go to the ALC status/products https://wiki.sophos.net/display/SophosCloud/EMP%3A+status-alc
// hence, if the product does not have subProducts it will be considered as top level component,
// when it has subProducts, it will be considered component Suite.
SubProducts ProductMetadata::combineSubProducts(const std::vector<ProductMetadata>& productsMetadata)
{
    Common::UtilityImpl::OrderedSet<ProductKey> orderedProducts;
    for( auto & productMetadata: productsMetadata)
    {
        SubProducts subProducts = productMetadata.subProducts();
        if( subProducts.empty())
        {
            orderedProducts.addElement(ProductKey{ productMetadata.getLine(), productMetadata.getVersion() });
        }
        else
        {
            for( auto & subProduct: subProducts)
            {
                orderedProducts.addElement(subProduct);
            }
        }
    }
    return orderedProducts.orderedElements();
}

bool SulDownloader::suldownloaderdata::operator==(const ProductKey& lh, const ProductKey& rh)
{
    return lh.m_line == rh.m_line && lh.m_version == rh.m_version;
}
