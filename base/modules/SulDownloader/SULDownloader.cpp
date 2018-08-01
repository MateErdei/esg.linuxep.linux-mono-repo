/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <iostream>
#include <algorithm>
#include <map>
#include <google/protobuf/util/json_util.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

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
        timeTracker.setSyncTime( TimeTracker::getCurrTime());

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
            return DownloadReport::Report(products, timeTracker);
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
        return DownloadReport::Report(products, timeTracker);

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
// TODO: future to go to the test suite
//    void create_fake_json_settings()
//    {
//        using SulDownloaderProto::ConfigurationSettings;
//        ConfigurationSettings settings;
//        Credentials credentials;
//
//        settings.add_sophosurls("http://ostia.eng.sophos/latest/Virt-vShield");
//        settings.add_updatecache("http://ostia.eng.sophos/latest/Virt-vShieldBroken");
//        settings.mutable_credential()->set_username("administrator");
//        settings.mutable_credential()->set_password("password");
//        settings.mutable_proxy()->set_url("noproxy:");
//        settings.mutable_proxy()->mutable_credential()->set_username("");
//        settings.mutable_proxy()->mutable_credential()->set_password("");
//        settings.set_baseversion("10");
//        settings.set_releasetag("RECOMMENDED");
//        settings.set_primary("FD6C1066-E190-4F44-AD0E-F107F36D9D40");
//        settings.add_fullnames("A845A8B5-6532-4EF1-B19E-1DB2B3CB73D1");
//        settings.add_prefixnames("A845A8B5");
//        settings.set_certificatepath("/home/pair/CLionProjects/everest-suldownloader/cmake-build-debug/certificates");
//
//
//        std::string json_output = MessageUtility::protoBuf2Json( settings );
//
//        std::cout << "Json serialized: " << json_output << std::endl;
//
//    }


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
