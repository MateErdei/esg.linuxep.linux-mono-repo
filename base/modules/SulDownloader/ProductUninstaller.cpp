/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProductUninstaller.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/ProcessImpl/ProcessImpl.h>

#include <algorithm>
#include <map>
#include <sstream>

namespace
{
    using namespace SulDownloader;
    std::string getLines(const std::vector<suldownloaderdata::DownloadedProduct>& downloadedProducts)
    {
        std::ostringstream ost;
        bool first = true;
        for (const auto& downloadedProduct : downloadedProducts)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                ost << ", ";
            }
            ost << downloadedProduct.getLine();
        }
        return ost.str();
    }
} // namespace

namespace SulDownloader
{
    std::vector<suldownloaderdata::DownloadedProduct> ProductUninstaller::removeProductsNotDownloaded(
        const std::vector<suldownloaderdata::DownloadedProduct>& downloadedProducts,
        suldownloaderdata::IWarehouseRepository& iWarehouseRepository)
    {
        std::vector<std::string> installedProducts = getInstalledProductPathsList();
        std::map<std::string, suldownloaderdata::DownloadedProduct> productsToRemove;

        for (auto& installedProduct : installedProducts)
        {
            std::string productLine = Common::FileSystem::basename(installedProduct);
            productLine = productLine.substr(0, productLine.find(".sh"));

            auto productItr = std::find_if(
                downloadedProducts.begin(),
                downloadedProducts.end(),
                [&productLine](const suldownloaderdata::DownloadedProduct& product) {
                    return product.getLine() == productLine;
                });

            if (productItr == downloadedProducts.end())
            {
                LOGWARN("Uninstalling plugin " << productLine << " since it was removed from warehouse");
                LOGINFO("Downloaded products: " << getLines(downloadedProducts));

                suldownloaderdata::ProductMetadata metadata;
                metadata.setLine(productLine);
                suldownloaderdata::DownloadedProduct product(metadata);
                product.setDistributePath(iWarehouseRepository.getProductDistributionPath(product));
                productsToRemove.insert(
                    std::pair<std::string, suldownloaderdata::DownloadedProduct>(installedProduct, product));
            }
        }

        return removeProducts(productsToRemove);
    }

    std::vector<std::string> ProductUninstaller::getInstalledProductPathsList()
    {
        std::vector<std::string> fileList;
        std::string filePath =
            Common::ApplicationConfiguration::applicationPathManager().getLocalUninstallSymLinkPath();
        if (Common::FileSystem::fileSystem()->isDirectory(filePath))
        {
            fileList = Common::FileSystem::fileSystem()->listFiles(filePath);
        }

        return fileList;
    }

    bool ProductUninstaller::prepareProductForDowngrade(const std::string& uninstallScript)
    {
        if (Common::FileSystem::fileSystem()->isFile(uninstallScript) &&
            Common::FileSystem::fileSystem()->isExecutable(uninstallScript))
        {
            auto process = ::Common::Process::createProcess();
            int exitCode = 0;

            std::stringstream errorMessage;
            try
            {
                process->exec(uninstallScript, {"--downgrade", "--force"}, {});
                auto output = process->output();
                LOGSUPPORT(output);
                exitCode = process->exitCode();

                if (exitCode != 0)
                {
                    errorMessage << "Failed to prepare product for downgrade, running '" << uninstallScript << "', code '" << exitCode
                                 << "' with error, Process did not complete successfully";
                }
                else
                {
                    return true;
                }
            }
            catch (Common::Process::IProcessException& ex)
            {
                errorMessage << "Failed to prepare product for downgrade'" << uninstallScript << "' with error, "
                             << ex.what();
            }
        }
        return false;
    }

    std::vector<suldownloaderdata::DownloadedProduct> ProductUninstaller::removeProducts(
        std::map<std::string, suldownloaderdata::DownloadedProduct> uninstallProductInfo)
    {
        std::vector<suldownloaderdata::DownloadedProduct> productsRemoved;

        for (auto& uninstallProduct : uninstallProductInfo)
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
                    errorMessage << "Failed to uninstall product '" << uninstallProduct.first << "', code '" << exitCode
                                 << "' with error, Process did not complete successfully";
                }
            }
            catch (Common::Process::IProcessException& ex)
            {
                errorMessage << "Failed to uninstall product '" << uninstallProduct.first << "' with error, "
                             << ex.what();
            }

            if (!errorMessage.str().empty())
            {
                suldownloaderdata::WarehouseError error;
                error.status = suldownloaderdata::WarehouseStatus::UNINSTALLFAILED;
                error.Description = errorMessage.str();
                uninstallProduct.second.setError(error);
                LOGERROR(errorMessage.str());
            }
            else
            {
                // given that the product has been removed. We also remove the copies in the distributed path.
                // This ensures that if the product were to appear again in the licence it will be installed.
                std::string distributePath = uninstallProduct.second.distributePath();
                LOGSUPPORT("Mark product uninstalled by removing distributed path: " << distributePath);
                try
                {
                    std::string localDistributionRepository =
                        Common::ApplicationConfiguration::applicationPathManager().getLocalDistributionRepository();
                    if (distributePath.find(localDistributionRepository) == std::string::npos)
                    {
                        throw std::logic_error(
                            "Received a path to remove that is not in the local distribution repository: " +
                            distributePath);
                    }
                    Common::FileSystem::fileSystem()->removeFileOrDirectory(distributePath);
                }
                catch (Common::FileSystem::IFileSystemException& ex)
                {
                    LOGWARN("Failed to remove path. Reason: " << ex.what());
                }
            }

            productsRemoved.push_back(uninstallProduct.second);
        }

        return productsRemoved;
    }
} // namespace SulDownloader