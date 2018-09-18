/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SXLMachineID.h"
#include "MACinfo.h"
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/sslimpl/Md5Calc.h>
#include <string>
#include <sstream>
#include <iostream>
namespace Common
{
    namespace OSUtilities
    {

        std::string SXLMachineID::getMachineID()
        {
            if ( Common::FileSystem::fileSystem()->isFile(machineIDPath()))
            {
                return Common::FileSystem::fileSystem()->readFile( machineIDPath() );
            }
            return std::string();
        }

        void SXLMachineID::createMachineID()
        {
            std::stringstream content;
            content << "sspl-machineid";
            for( auto mac : sortedSystemMACs())
            {
                content << mac;
            }
            std::string md5hash = Common::sslimpl::md5(content.str());
            std::string re_hash = Common::sslimpl::md5(md5hash);
            Common::FileSystem::fileSystem()->writeFile(machineIDPath(), re_hash);
        }

        std::string SXLMachineID::machineIDPath() const
        {
            return Common::FileSystem::join(
                    Common::ApplicationConfiguration::applicationPathManager().sophosInstall(),
                    "base/etc/machine_id.txt");
        }

        std::string SXLMachineID::fetchMachineIdAndCreateIfNecessary()
        {
            std::string machineID = getMachineID();
            if ( machineID.empty())
            {
                createMachineID();
                machineID = getMachineID();
            }
            return machineID;
        }

        //It is meant to be used in the installer, hence, it does not use log but standard error
        int mainEntry(int argc, char* argv[])
        {
            if( argc != 2)
            {
                std::cerr << "Invalid argument. Usage: ./machineid /opt/sophos-spl" << std::endl;
                return 1;
            }

            std::string sophosRootPath = argv[1];
            if ( !Common::FileSystem::fileSystem()->isDirectory(sophosRootPath))
            {
                std::cerr << "Invalid argument. Argument does not point to a directory" << std::endl;
                return 2;
            }

            Common::ApplicationConfiguration::applicationConfiguration().setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, sophosRootPath);

            SXLMachineID sxlMachineID;
            try
            {
                if ( sxlMachineID.getMachineID() == "")
                {
                    sxlMachineID.createMachineID();
                }

            }catch (std::exception& ex)
            {
                std::cerr << "Failed to create machine id. Error: " << ex.what() << std::endl;
                return 3;
            }
            return 0;
        }

    }
}