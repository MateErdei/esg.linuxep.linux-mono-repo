// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "SXLMachineID.h"

#include "MACinfo.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/SslImpl/Digest.h"
#include "Common/UtilityImpl/ProjectNames.h"

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
} // namespace Common::OSUtilitiesImpl