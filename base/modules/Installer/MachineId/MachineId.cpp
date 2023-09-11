// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "MachineId.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/OSUtilitiesImpl/MACinfo.h"
#include "Common/OSUtilitiesImpl/SXLMachineID.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <iostream>

namespace Installer::MachineId
{
    // It is meant to be used in the installer, hence, it does not use log but standard error
    int mainEntry(int argc, char* argv[])
    {
        if (argc != 2)
        {
            std::cerr << "Invalid argument. Usage: ./machineid /opt/sophos-spl OR ./machineid --dump-mac-addresses"
                      << std::endl;
            return 1;
        }
        std::string argument;
        try
        {
            argument = Common::UtilityImpl::StringUtils::checkAndConstruct(argv[1]);
        }
        catch (std::invalid_argument& e)
        {
            std::cerr << "Invalid argument. Argument is not utf8: " << argument << std::endl;
            return 2;
        }

        if (argument == "--dump-mac-addresses")
        {
            std::stringstream output;
            for (const std::string& mac : Common::OSUtilitiesImpl::sortedSystemMACs())
            {
                output << mac << "\n";
            }
            std::cout << output.str();
            return 0;
        }
        else if (!Common::FileSystem::fileSystem()->isDirectory(argument))
        {
            std::cerr << "Invalid argument. Argument does not point to a directory" << std::endl;
            return 2;
        }

        Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, argument);

        Common::OSUtilitiesImpl::SXLMachineID sxlMachineID;
        try
        {
            if (sxlMachineID.getMachineID().empty())
            {
                sxlMachineID.createMachineIDFile();
            }
        }
        catch (std::exception& ex)
        {
            std::cerr << "Failed to create machine id. Error: " << ex.what() << std::endl;
            return 3;
        }
        return 0;
    }
} // namespace Installer::MachineId
