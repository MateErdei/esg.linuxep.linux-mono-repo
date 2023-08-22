// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "SXLMachineID.h"

#include "MACinfo.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/SslImpl/Digest.h"
#include "Common/UtilityImpl/ProjectNames.h"
#include "Common/UtilityImpl/StringUtils.h"
#include <sys/stat.h>

#include <iostream>
#include <string>

namespace Common::OSUtilitiesImpl
{
    std::string SXLMachineID::getMachineID()
    {
        if (Common::FileSystem::fileSystem()->isFile(machineIDPath()))
        {
            return Common::FileSystem::fileSystem()->readFile(machineIDPath());
        }
        return std::string();
    }

    std::string SXLMachineID::generateMachineID()
    {
        std::stringstream content;
        content << "sspl-machineid";
        for (const auto& mac : sortedSystemMACs())
        {
            content << mac;
        }
        using namespace Common::SslImpl;
        std::string md5hash = calculateDigest(Digest::md5, content.str());
        std::string re_hash = calculateDigest(Digest::md5, md5hash);
        return re_hash;
    }

    void SXLMachineID::createMachineIDFile()
    {
        Common::FileSystem::fileSystem()->writeFile(machineIDPath(), generateMachineID());
        Common::FileSystem::filePermissions()->chmod(machineIDPath(), S_IRUSR | S_IWUSR | S_IRGRP); // NOLINT
        Common::FileSystem::filePermissions()->chown(machineIDPath(), "root", sophos::group());
    }

    std::string SXLMachineID::machineIDPath() const
    {
        return Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().sophosInstall(), "base/etc/machine_id.txt");
    }

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
            argument = UtilityImpl::StringUtils::checkAndConstruct(argv[1]);
        }
        catch (std::invalid_argument& e)
        {
            std::cerr << "Invalid argument. Argument is not utf8: " << argument << std::endl;
            return 2;
        }

        if (argument == "--dump-mac-addresses")
        {
            std::stringstream output;
            for (const std::string& mac : sortedSystemMACs())
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

        SXLMachineID sxlMachineID;
        try
        {
            if (sxlMachineID.getMachineID() == "")
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
} // namespace Common::OSUtilitiesImpl