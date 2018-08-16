/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "ProductUninstaller.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/Process/IProcess.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <Common/Process/IProcessException.h>

#include <algorithm>
#include <map>
#include <sstream>

namespace SulDownloader
{

    std::vector<DownloadedProduct> ProductUninstaller::removeProductsNotDownloaded(
            const std::vector<DownloadedProduct> &downloadedProducts)
    {
        std::vector<std::string> installedProducts = getInstalledProductPathsList();
        std::map<std::string, DownloadedProduct> productsToRemove;

        for(auto& installedProduct : installedProducts)
        {
            std::string productLine = Common::FileSystem::basename(installedProduct);
            productLine = productLine.substr(0, productLine.find(".sh"));

            auto productItr = std::find_if(downloadedProducts.begin(), downloadedProducts.end(),
                    [&productLine](const DownloadedProduct& product) {return product.getLine() == productLine;});

            if(productItr == downloadedProducts.end())
            {
                ProductMetadata metadata;
                metadata.setLine(productLine);
                DownloadedProduct product(metadata);
                productsToRemove.insert(std::pair<std::string, DownloadedProduct>(installedProduct, product));
            }
        }

        return removeProducts(productsToRemove);
    }

    std::vector<std::string> ProductUninstaller::getInstalledProductPathsList()
    {
        std::vector<std::string> fileList;
        std::string filePath = Common::ApplicationConfiguration::applicationPathManager().getLocalUninstallSymLinkPath();
        if(Common::FileSystem::fileSystem()->isDirectory(filePath))
        {
            fileList = Common::FileSystem::fileSystem()->listFiles(filePath);
        }

        return fileList;
    }

    std::vector<DownloadedProduct> ProductUninstaller::removeProducts(std::map<std::string, DownloadedProduct> uninstallProductInfo)
    {
        std::vector<DownloadedProduct> productsRemoved;

        for (auto &uninstallProduct : uninstallProductInfo)
        {
            auto process = ::Common::Process::createProcess();
            int exitCode = 0;

            uninstallProduct.second.setProductIsBeingUninstalled(true);

            std::stringstream errorMessage;
            try
            {
                process->exec(uninstallProduct.first, {}, {});
                auto output = process->output();
                LOGSUPPORT(output);
                exitCode = process->exitCode();

                if (exitCode != 0)
                {
                    errorMessage << "Failed to uninstall product '" << uninstallProduct.first << "', code '" << exitCode << "' with error, Process did not complete successfully";
                }
            }
            catch (Common::Process::IProcessException &ex)
            {
                errorMessage << "Failed to uninstall product '" << uninstallProduct.first << "' with error, " << ex.what();
            }

            if (!errorMessage.str().empty())
            {
                WarehouseError error;
                error.status = WarehouseStatus::UNINSTALLFAILED;
                error.Description = errorMessage.str();
                uninstallProduct.second.setError(error);
                LOGERROR(errorMessage.str());
            }

            productsRemoved.push_back(uninstallProduct.second);
        }

        return productsRemoved;
    }
}