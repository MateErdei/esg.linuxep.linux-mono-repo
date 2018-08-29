/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "SULRaii.h"
#include "WarehouseRepository.h"
#include "WarehouseRepositoryFactory.h"
#include "Logger.h"
#include "ProductUninstaller.h"

#include <SulDownloader/suldownloaderdata/DownloadReport.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <SulDownloader/suldownloaderdata/TimeTracker.h>

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Logging/FileLoggingSetup.h>

#include <cassert>
#include <algorithm>

using namespace SulDownloader::suldownloaderdata;

namespace
{
    bool hasError( const std::vector<SulDownloader::suldownloaderdata::DownloadedProduct> & products )
    {
        for( const auto & product: products)
        {
            if ( product.hasError())
            {
                return true;
            }
        }
        return false;
    }
}

namespace SulDownloader
{
    using namespace Common::UtilityImpl;

    bool forceInstallOfProduct(const DownloadedProduct& product, const DownloadReport& previousDownloadReport)
    {
        //If the previous install failed with unspecified or download failed we have no clear
        //information on the state of the distribution folder. Therefore safest approach is to force a reinstall.
        if (previousDownloadReport.getStatus() == WarehouseStatus::UNSPECIFIED || previousDownloadReport.getStatus() == WarehouseStatus::DOWNLOADFAILED )
        {
            return true;
        }

        const std::vector<ProductReport>& productReports = previousDownloadReport.getProducts();
        auto productReportItr = std::find_if(productReports.begin(), productReports.end(), [&product](const ProductReport & report){return report.rigidName == product.getLine();});

        if (productReportItr != productReports.end())
        {
            if (productReportItr->productStatus == ProductReport::ProductStatus::InstallFailed ||
                productReportItr->productStatus == ProductReport::ProductStatus::SyncFailed)
            {
                return true;
            }
        }
        return false;
    }


    DownloadReport runSULDownloader( const ConfigurationData & configurationData, const DownloadReport& previousDownloadReport)
    {
        SULInit init;
        assert( configurationData.isVerified());
        TimeTracker timeTracker;
        timeTracker.setStartTime( TimeUtils::getCurrTime());

        // connect and read metadata
        auto warehouseRepository = WarehouseRepositoryFactory::instance().fetchConnectedWarehouseRepository(configurationData);

        if ( warehouseRepository->hasError())
        {
            return DownloadReport::Report(*warehouseRepository, timeTracker);
        }

        // apply product selection and download the needed products
        auto productSelection = ProductSelection::CreateProductSelection(configurationData);
        warehouseRepository->synchronize(productSelection);


        if ( warehouseRepository->hasError())
        {
            return DownloadReport::Report(*warehouseRepository, timeTracker);
        }

        warehouseRepository->distribute();

        if ( warehouseRepository->hasError())
        {
            return DownloadReport::Report(*warehouseRepository, timeTracker);
        }


        auto products = warehouseRepository->getProducts();
        std::string sourceURL = warehouseRepository->getSourceURL();
        std::string rootcaPath = Common::FileSystem::join(configurationData.getCertificatePath(), "rootca.crt");

        // Mark which products need to be forced to re/install.
        for(auto & product : products)
        {
            if(configurationData.getForceReinstallAllProducts() || forceInstallOfProduct(product, previousDownloadReport))
            {
                product.setForceProductReinstall(true);
            }
        }

        // Only need to verify the products which the install.sh will be called on.
        for( auto & product: products)
        {
            if(product.productHasChanged() || product.forceProductReinstall())
            {
                product.verify(rootcaPath);
            }
        }

        if ( hasError(products))
        {
            return DownloadReport::Report(sourceURL, products, &timeTracker, DownloadReport::VerifyState::VerifyFailed);
        }

        // design decision: do not install if any error happens before this time.
        // try to install all products and report error for those that failed (if any)
        for( auto & product: products)
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

        // Note: Should only get here if Download has been successful, if no products are downloaded then
        // a warehouse error should have been generated, preventing getting this far, therefore preventing
        // un-installation of all products.
        SulDownloader::ProductUninstaller uninstallManager;
        std::vector<DownloadedProduct> uninstalledProducts = uninstallManager.removeProductsNotDownloaded(products);
        for (auto &uninstalledProduct : uninstalledProducts ){
            products.push_back(uninstalledProduct);
        }

        timeTracker.setFinishedTime( TimeUtils::getCurrTime());

        // if any error happened during installation, it reports correctly.
        // the report also contains the successful ones.
        return DownloadReport::Report(sourceURL, products, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    }

    std::tuple<int, std::string> configAndRunDownloader(const std::string& settingsString, const std::string& previousReportData)
    {
        try
        {
            ConfigurationData configurationData = ConfigurationData::fromJsonSettings( settingsString);

            if(!configurationData.verifySettingsAreValid())
            {
                throw SulDownloaderException("Configuration data is invalid");
            }

            // If there is no previous download report, or if the download report fails to be read correctly
            // assume default download and install behaviour. i.e. install if file set has changed.
            // If overriding is required then add code to set configurationData.setForceReinstallAllProducts(true)
            DownloadReport previousDownloadReport =  DownloadReport::Report("Not assigned");

            try
            {
                if(!previousReportData.empty())
                {
                    previousDownloadReport = DownloadReport::toReport(previousReportData);
                }
            }
            catch(SulDownloaderException &ex)
            {
                LOGWARN("Failed to load previous report data");
            }

            auto report = runSULDownloader(configurationData, previousDownloadReport);

            return DownloadReport::CodeAndSerialize(report);

        }
        catch( std::exception & ex )
        {
            LOGERROR(ex.what());
            auto report = DownloadReport::Report("SulDownloader failed.");
            return DownloadReport::CodeAndSerialize( report);
        }

    };

    std::string getPreviousDownloadReportData(const std::string& outputParentPath)
    {
        std::vector<std::string> filesInReportDirectory = Common::FileSystem::fileSystem()->listFiles(outputParentPath);


        // Filter file list to make sure all the files are report files based on file name
        std::vector<std::string>previousReportFiles;
        std::string startPattern("report");
        std::string endPattern(".json");

        for(auto& file : filesInReportDirectory)
        {
            std::string fileName = Common::FileSystem::basename(file);

            // make sure file name begins with 'report' and ends with .'json'
            if(fileName.find(startPattern) == 0 && fileName.find(endPattern) == (fileName.length() - endPattern.length()))
            {
                previousReportFiles.push_back(file);
            }
        }

        std::string previousDownloadReport;

        if(!previousReportFiles.empty())
        {
            std::sort(previousReportFiles.begin(), previousReportFiles.end());

            std::string previousReportFileName = previousReportFiles.back();
            try
            {
                previousDownloadReport = Common::FileSystem::fileSystem()->readFile(previousReportFileName);
            }
            catch(const Common::FileSystem::IFileSystemException& ex)
            {
                LOGERROR("Failed to load previous download report file: " << previousReportFileName);
            }
        }

        return previousDownloadReport;
    }

    int fileEntriesAndRunDownloader( const std::string & inputFilePath, const std::string & outputFilePath )
    {
        auto fileSystem = Common::FileSystem::fileSystem();

        std::string settingsString = fileSystem->readFile(inputFilePath);

        // check can create the output
        if ( fileSystem->isDirectory(outputFilePath))
        {
            LOGERROR("Output path given is directory: " << outputFilePath);
            throw SulDownloaderException("Output file path cannot be a directory");
        }

        std::string outputParentPath = Common::FileSystem::dirName(outputFilePath);
        if ( !fileSystem->isDirectory(outputParentPath))
        {
            LOGERROR("The directory of the output path does not exists: " << outputParentPath);
            throw SulDownloaderException( "The directory to write the output file must exist.");
        }

        std::string previousReportData = getPreviousDownloadReportData(outputParentPath);

        auto result = configAndRunDownloader(settingsString, previousReportData);
        std::string tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        LOGSUPPORT("Generate the report file: " << outputFilePath << " using temp directory " << tempDir);


        fileSystem->writeFileAtomically(outputFilePath, std::get<1>(result), tempDir);

        return std::get<0>(result);
    }

    int main_entry( int argc, char * argv[])
    {
        // Configure logging
        Common::Logging::FileLoggingSetup loggerSetup("suldownloader");

        //Process command line args
        if(argc < 3)
        {
            LOGERROR("Error, invalid command line arguments. Usage: SULDownloader inputpath outputpath");
            return -1;
        }

        std::string inputPath = argv[1];
        std::string outputPath = argv[2];

        try
        {
            return fileEntriesAndRunDownloader(inputPath, outputPath);
        }// failed or unable to either read or to write files
        catch( std::exception & ex)
        {
            LOGERROR(ex.what());
            return -2;
        }
        catch( ... )
        {
            LOGERROR( "Unknown error");
            return -3;
        }


    }

}
