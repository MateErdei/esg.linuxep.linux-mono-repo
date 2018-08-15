/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "DownloadedProduct.h"

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
        std::vector<DownloadedProduct> removeProductsNotDownloaded(const std::vector<DownloadedProduct> &downloadedProducts);

    private:
        std::vector<std::string> getInstalledProductPathsList();
        std::vector<DownloadedProduct> removeProducts(std::map<std::string, DownloadedProduct> uninstallProductInfo);

    };


}