// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IRepository.h"
#include "ProductMetadata.h"

#include "Common/DownloadReport/ProductInfo.h"

#include <unordered_map>

namespace SulDownloader::suldownloaderdata
{
    class CatalogueInfo
    {
    public:
        static std::vector<Common::DownloadReport::ProductInfo> calculatedListProducts(
            const std::vector<suldownloaderdata::DownloadedProduct>&,
            const suldownloaderdata::CatalogueInfo&);
        void addInfo(const std::string& line, const std::string& version, const std::string& productName);
        std::string productName(const std::string& productLine, const std::string& productVersion) const;

        void reset();

    private:
        using Key = ProductKey;
        std::unordered_map<Key, std::string> m_catalogue;
    };
} // namespace SulDownloader::suldownloaderdata
