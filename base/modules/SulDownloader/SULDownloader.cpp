/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProductUninstaller.h"
#include "SULRaii.h"
#include "WarehouseRepository.h"
#include "WarehouseRepositoryFactory.h"

#include "suldownloaderdata/Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IPidLockFileUtils.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/UtilityImpl/ProjectNames.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/UtilityImpl/UniformIntDistribution.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/ConfigurationDataUtil.h>
#include <SulDownloader/suldownloaderdata/DownloadReport.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <SulDownloader/suldownloaderdata/TimeTracker.h>
#include <sys/stat.h>

#include <algorithm>
#include <cassert>

using namespace SulDownloader::suldownloaderdata;

namespace
{
    bool hasError(const std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>& products)
    {
        for (const auto& product : products)
        {
            if (product.hasError())
            {
                return true;
            }
        }
        return false;
    }

    void WriteAtomicFileWithReadAndWriteToGroup(
        const std::string& outputFilePath,
        const std::string& content,
        const std::string& tempDir)
    {
        Common::FileSystem::createAtomicFileToSophosUser(content, outputFilePath, tempDir);
    }

} // namespace

namespace SulDownloader
{
    using namespace Common::UtilityImpl;

    bool forceInstallOfProduct(const DownloadedProduct& product, const DownloadReport& previousDownloadReport)
    {
        // If the previous install failed with unspecified or download failed we have no clear
        // information on the state of the distribution folder. Therefore safest approach is to force a reinstall.
        if (previousDownloadReport.getStatus() == WarehouseStatus::UNSPECIFIED ||
            previousDownloadReport.getStatus() == WarehouseStatus::DOWNLOADFAILED)
        {
            LOGDEBUG("Force reinstall because previous report failed.");
            return true;
        }

        const std::vector<ProductReport>& productReports = previousDownloadReport.getProducts();
        auto productReportItr =
            std::find_if(productReports.begin(), productReports.end(), [&product](const ProductReport& report) {
                return report.rigidName == product.getLine();
            });

        if (productReportItr != productReports.end())
        {
            if (productReportItr->productStatus == ProductReport::ProductStatus::InstallFailed ||
                productReportItr->productStatus == ProductReport::ProductStatus::SyncFailed ||
                productReportItr->productStatus == ProductReport::ProductStatus::VerifyFailed)
            {
                LOGDEBUG("Force reinstall because this specific product failed.");
                return true;
            }
        }
        return false;
    }

    DownloadReport runSULDownloader(
        const ConfigurationData& configurationData,
        const ConfigurationData& previousConfigurationData,
        const DownloadReport& previousDownloadReport)
    {
        SULInit init;
        assert(configurationData.isVerified());
        TimeTracker timeTracker;
        timeTracker.setStartTime(TimeUtils::getCurrTime());

        // connect and read metadata
        auto warehouseRepository =
            WarehouseRepositoryFactory::instance().fetchConnectedWarehouseRepository(configurationData);

        if (warehouseRepository->hasError())
        {
            return DownloadReport::Report(*warehouseRepository, timeTracker);
        }

        // apply product selection and download the needed products
        auto productSelection = ProductSelection::CreateProductSelection(configurationData);
        warehouseRepository->synchronize(productSelection);

        if (warehouseRepository->hasError())
        {
            return DownloadReport::Report(*warehouseRepository, timeTracker);
        }

        warehouseRepository->distribute();

        if (warehouseRepository->hasError())
        {
            return DownloadReport::Report(*warehouseRepository, timeTracker);
        }

        auto products = warehouseRepository->getProducts();
        std::string sourceURL = warehouseRepository->getSourceURL();

        // Mark which products need to be forced to re/install.
        bool forceReinstallAllProducts =
            SulDownloader::suldownloaderdata::ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(
                configurationData, previousConfigurationData, false);

        for(auto& product : products)
        {
            std::string rigidName = product.getProductMetadata().getLine();
            std::string filePath = Common::FileSystem::join( product.distributePath(),"VERSION.ini");
            Common::ApplicationConfiguration::applicationPathManager().getVersionIniFileForComponent(filePath);
        }

        for (auto& product : products)
        {
            bool forceReinstallThisProduct = forceInstallOfProduct(product, previousDownloadReport);

            if (forceReinstallAllProducts || forceReinstallThisProduct)
            {
                LOGSUPPORT(
                    "Mark product to be reinstalled. Reason: AllProducts: "
                    << forceReinstallAllProducts << ", This Product: " << forceReinstallThisProduct
                    << ". Product = " << product.getLine());
                product.setForceProductReinstall(true);
            }
        }
        LOGSUPPORT("Checking signature.");
        // Only need to verify the products which the install.sh will be called on.
        for (auto& product : products)
        {
            if (product.productHasChanged() || product.forceProductReinstall())
            {
                product.verify(configurationData);
            }
        }

        if (hasError(products))
        {
            LOGWARN("Verification of the downloaded products failed.");
            return DownloadReport::Report(
                sourceURL,
                products,
                {},
                warehouseRepository->listInstalledSubscriptions(),
                &timeTracker,
                DownloadReport::VerifyState::VerifyFailed);
        }

        // Note: Should only get here if Download has been successful, if no products are downloaded then
        // a warehouse error should have been generated, preventing getting this far, therefore preventing
        // un-installation of all products.
        // We need to perform un-install of any components not downloaded first to prevent a race condition
        // between component registration and product un-installation when upgrading Base.
        SulDownloader::ProductUninstaller uninstallManager;
        std::vector<DownloadedProduct> uninstalledProducts =
                uninstallManager.removeProductsNotDownloaded(products, *warehouseRepository);
        for (auto& uninstalledProduct : uninstalledProducts)
        {
            products.push_back(uninstalledProduct);
        }

        // design decision: do not install if any error happens before this time.
        // try to install all products and report error for those that failed (if any)
        for (auto& product : products)
        {
            if (product.productHasChanged() || product.forceProductReinstall())
            {
                product.install(configurationData.getInstallArguments());
            }
            else
            {
                LOGINFO("Downloaded Product line: '" << product.getLine() << "' is up to date.");
            }
        }

        timeTracker.setFinishedTime(TimeUtils::getCurrTime());

        // if any error happened during installation, it reports correctly.
        // the report also contains the successful ones.
        return DownloadReport::Report(
            sourceURL,
            products,
            warehouseRepository->listInstalledProducts(),
            warehouseRepository->listInstalledSubscriptions(),
            &timeTracker,
            DownloadReport::VerifyState::VerifyCorrect);
    }

    std::tuple<int, std::string> configAndRunDownloader(
        const std::string& settingsString,
        const std::string& previousSettingString,
        const std::string& previousReportData)
    {
        try
        {
            ConfigurationData configurationData = ConfigurationData::fromJsonSettings(settingsString);

            ConfigurationData previousConfigurationData;

            if (!previousSettingString.empty())
            {
                previousConfigurationData = ConfigurationData::fromJsonSettings(previousSettingString);
                if (!previousConfigurationData.verifySettingsAreValid())
                {
                    LOGDEBUG("No previous configuration data provided");
                }
            }

            if (!configurationData.verifySettingsAreValid())
            {
                throw SulDownloaderException("Configuration data is invalid");
            }

            // If there is no previous download report, or if the download report fails to be read correctly
            // assume default download and install behaviour. i.e. install if file set has changed.
            // If overriding is required then add code to set configurationData.setForceReinstallAllProducts(true)
            DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

            try
            {
                if (!previousReportData.empty())
                {
                    previousDownloadReport = DownloadReport::toReport(previousReportData);
                }
            }
            catch (const SulDownloaderException& ex)
            {
                LOGWARN("Failed to load previous report data");
                LOGSUPPORT(ex.what());
            }

            auto report = runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);

            return DownloadReport::CodeAndSerialize(report);
        }
        catch (std::exception& ex)
        {
            LOGERROR(ex.what());
            auto report = DownloadReport::Report("SulDownloader failed.");
            return DownloadReport::CodeAndSerialize(report);
        }
    }

    std::string getPreviousDownloadReportData(const std::string& outputParentPath)
    {
        // Filter file list to make sure all the files are report files based on file name
        auto previousReportFiles = DownloadReport::listOfAllPreviousReports(outputParentPath);

        std::string previousDownloadReport;

        if (!previousReportFiles.empty())
        {
            std::sort(previousReportFiles.begin(), previousReportFiles.end());

            std::string previousReportFileName = previousReportFiles.back();
            try
            {
                previousDownloadReport = Common::FileSystem::fileSystem()->readFile(previousReportFileName);
            }
            catch (const Common::FileSystem::IFileSystemException& ex)
            {
                LOGERROR("Failed to load previous download report file: " << previousReportFileName);
                LOGSUPPORT(ex.what());
            }
        }

        return previousDownloadReport;
    }

    int fileEntriesAndRunDownloader(const std::string& inputFilePath, const std::string& outputFilePath)
    {
        auto fileSystem = Common::FileSystem::fileSystem();

        // previous setting data will be based on location of inputFilePath.
        Path previousSettingFilePath = Common::FileSystem::join(
            Common::FileSystem::dirName(inputFilePath),
            Common::ApplicationConfiguration::applicationPathManager().getPreviousUpdateConfigFileName());

        std::string settingsString = fileSystem->readFile(inputFilePath);

        std::string previousSettingsString("");

        if (fileSystem->isFile(previousSettingFilePath))
        {
            try
            {
                previousSettingsString = fileSystem->readFile(previousSettingFilePath);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to read previous update configuration file.");
            }
        }

        // check can create the output
        if (fileSystem->isDirectory(outputFilePath))
        {
            LOGERROR("Output path given is directory: " << outputFilePath);
            throw SulDownloaderException("Output file path cannot be a directory");
        }

        std::string outputParentPath = Common::FileSystem::dirName(outputFilePath);
        if (!fileSystem->isDirectory(outputParentPath))
        {
            LOGERROR("The directory of the output path does not exists: " << outputParentPath);
            throw SulDownloaderException("The directory to write the output file must exist.");
        }

        std::string previousReportData = getPreviousDownloadReportData(outputParentPath);
        int exitCode;
        std::string jsonReport;
        std::tie(exitCode, jsonReport) =
            configAndRunDownloader(settingsString, previousSettingsString, previousReportData);

        if (exitCode == 0)
        {
            LOGINFO("Update success");
        }
        else
        {
            LOGWARN("Update failed, with code: " << exitCode);
        }

        std::string tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        LOGINFO("Generating the report file in: " << outputParentPath);

        WriteAtomicFileWithReadAndWriteToGroup(outputFilePath, jsonReport, tempDir);

        return exitCode;
    }

    int main_entry(int argc, char* argv[])
    {
        umask(S_IRWXG | S_IRWXO);
        // Configure logging
        Common::Logging::FileLoggingSetup loggerSetup("suldownloader");
        std::unique_ptr<Common::FileSystem::ILockFileHolder> pidLock;
        try
        {
            pidLock = Common::FileSystem::acquireLockFile(
                Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath());
        }
        catch (const std::system_error& ex)
        {
            LOGERROR(ex.what());
            return ex.code().value();
        }

        // Process command line args
        if (argc < 3)
        {
            LOGERROR("Error, invalid command line arguments. Usage: SULDownloader inputpath outputpath");
            return -1;
        }

        std::string inputPath = argv[1];
        std::string outputPath = argv[2];

        try
        {
            return fileEntriesAndRunDownloader(inputPath, outputPath);
        } // failed or unable to either read or to write files
        catch (std::exception& ex)
        {
            LOGERROR("Main exception: " << ex.what());
            return -2;
        }
        catch (...)
        {
            LOGERROR("Unknown error");
            return -3;
        }
    }

} // namespace SulDownloader
