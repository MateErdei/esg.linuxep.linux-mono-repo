/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Tag.h"

#include <string>
#include <vector>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        struct ProductKey
        {
            std::string m_line;
            std::string m_version;
        };

        bool operator==(const ProductKey& lh, const ProductKey & rh);

        using SubProducts = std::vector<ProductKey>;

        /**
         * Product metadata from the warehouse.
         */
        class ProductMetadata
        {
        public:
            static SubProducts extractSubProductsFromSulSubComponents( const std::vector<std::string>&  sulSubComponents);
            static ProductKey extractProductKeyFromSubComponent( const std::string & sulSubComponent);
            static SubProducts combineSubProducts( const std::vector<ProductMetadata>& );
            const std::string& getLine() const;

            void setLine(const std::string& line);

            const std::string& getName() const;

            void setName(const std::string& name);

            void setTags(const std::vector<Tag>& tags);

            bool hasTag(const std::string& releaseTag) const;
            const std::vector<Tag> tags() const;

            std::string getBaseVersion() const;

            const std::string& getVersion() const;

            void setVersion(const std::string& version);
            void setBaseVersion( const std::string & baseVersion);

            void setDefaultHomePath(const std::string& defaultHomeFolder);
            void setFeatures(const std::vector<std::string> & features);
            const std::vector<std::string>& getFeatures() const;

            void setSubProduts( const SubProducts & subProducts);
            const SubProducts& subProducts() const;


        private:
            std::vector<Tag> m_tags;
            std::string m_line;
            std::string m_name;
            std::string m_version;
            std::string m_baseVersion;
            std::string m_defaultHomeFolder;
            std::vector<std::string> m_features;
            SubProducts m_subProducts;
        };

    } // namespace suldownloaderdata
} // namespace SulDownloader

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
