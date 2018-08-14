/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>
#include <map>
#include "DownloadedProduct.h"

namespace SulDownloader
{
    class UninstallManager
    {
    public:
        UninstallManager() = default;
        ~UninstallManager() = default;
        std::vector<DownloadedProduct> performCleanUp(const std::vector<DownloadedProduct> &downloadedProducts);

    private:
        std::vector<std::string> getInstalledProductPathsList();
        std::vector<DownloadedProduct> removeProducts(std::map<std::string, DownloadedProduct> uninstallProductInfo);

    };


}