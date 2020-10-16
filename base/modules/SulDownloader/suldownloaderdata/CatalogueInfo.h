/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IWarehouseRepository.h"
#include "ProductMetadata.h"

#include <unordered_map>
namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class CatalogueInfo
        {
        public:
            static std::vector<suldownloaderdata::ProductInfo> calculatedListProducts(
                const std::vector<suldownloaderdata::DownloadedProduct>&,
                const suldownloaderdata::CatalogueInfo&);
            void addInfo(const std::string& line, const std::string& version, const std::string& productName);
            std::string productName(const std::string& productLine, const std::string& productVersion) const;

            void reset();

        private:
            using Key = ProductKey;
            std::unordered_map<Key, std::string> m_catalogue;
        };
    } // namespace suldownloaderdata
} // namespace SulDownloader
