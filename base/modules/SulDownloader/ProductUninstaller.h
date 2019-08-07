/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/IWarehouseRepository.h>

#include <map>
#include <string>
#include <vector>

namespace SulDownloader
{
    class ProductUninstaller
    {
    public:
        ProductUninstaller() = default;
        ~ProductUninstaller() = default;
        /**
         * @brief This function will run the uninstall scripts for any product installed that is not in
         *        the list of downloaded products.
         * @param downloadedProducts
         * @return List of products that have been uninstalled
         */
        std::vector<suldownloaderdata::DownloadedProduct> removeProductsNotDownloaded(
            const std::vector<suldownloaderdata::DownloadedProduct>& downloadedProducts,
            suldownloaderdata::IWarehouseRepository& iWarehouseRepository);

    private:
        std::vector<std::string> getInstalledProductPathsList();
        std::vector<suldownloaderdata::DownloadedProduct> removeProducts(
            std::map<std::string, suldownloaderdata::DownloadedProduct> uninstallProductInfo);
    };

} // namespace SulDownloader