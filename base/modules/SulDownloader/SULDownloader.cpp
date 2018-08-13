/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "SULRaii.h"
#include "DownloadReport.h"
#include "ConfigurationData.h"
#include "WarehouseRepository.h"
#include "ProductSelection.h"
#include "DownloadedProduct.h"
#include "SulDownloaderException.h"
#include "TimeTracker.h"
#include "Common/FileSystem/IFileSystem.h"
#include "WarehouseRepositoryFactory.h"
#include "Logger.h"
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>


namespace
{
    bool hasError( const std::vector<SulDownloader::DownloadedProduct> & products )
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
    DownloadReport runSULDownloader( const ConfigurationData & configurationData)
    {
        SULInit init;
        assert( configurationData.isVerified());
        TimeTracker timeTracker;
        timeTracker.setStartTime( TimeTracker::getCurrTime());

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
        std::string rootcaPath = Common::FileSystem::join(configurationData.getCertificatePath(), "rootca.crt");
        for( auto & product: products)
        {
            product.verify(rootcaPath);
        }

        if ( hasError(products))
        {
            return DownloadReport::Report(products, &timeTracker, DownloadReport::VerifyState::VerifyFailed);
        }

        // design decision: do not install if any error happens before this time.
        // try to install all products and report error for those that failed (if any)
        for( auto & product: products)
        {
            if (product.productHasChanged())
            {
                product.install(configurationData.getInstallArguments());
            }
            else
            {
                LOGINFO("Downloaded Product line: '" << product.getLine() << "' is up to date.");
            }
        }

        timeTracker.setFinishedTime( TimeTracker::getCurrTime());

        // if any error happened during installation, it reports correctly.
        // the report also contains the successful ones.
        return DownloadReport::Report(products, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    }

    std::tuple<int, std::string> configAndRunDownloader( const std::string & settingsString)
    {
        try
        {
            ConfigurationData configurationData = ConfigurationData::fromJsonSettings( settingsString);

            if(!configurationData.verifySettingsAreValid())
            {
                throw SulDownloaderException("Configuration data is invalid");
            }
            auto report = runSULDownloader(configurationData);

            return DownloadReport::CodeAndSerialize(report);

        }
        catch( std::exception & ex )
        {
            LOGERROR(ex.what());
            auto report = DownloadReport::Report("SulDownloader failed.");
            return DownloadReport::CodeAndSerialize( report);
        }

    };

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

        auto result = configAndRunDownloader(settingsString);
        std::string tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        LOGSUPPORT("Generate the report file: " << outputFilePath << " using temp directory " << tempDir);

        fileSystem->writeFileAtomically(outputFilePath, std::get<1>(result), tempDir);

        return std::get<0>(result);
    }



    int main_entry( int argc, char * argv[])
    {

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
