/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NamedScanConfig.h"

#define AUTO_FD_IMPLICIT_INT
#include <datatypes/AutoFd.h>

#include <capnp/serialize.h>

#include <fstream>

#include <fcntl.h>

using namespace avscanner::avscannerimpl;

NamedScanConfig::NamedScanConfig(const Sophos::ssplav::NamedScan::Reader& namedScanConfig)
    : m_scanName(namedScanConfig.getName())
    , m_scanArchives(namedScanConfig.getScanArchives())
    , m_scanHardDisc(namedScanConfig.getScanHardDrives())
    , m_scanOptical(namedScanConfig.getScanCDDVDDrives())
    , m_scanNetwork(namedScanConfig.getScanNetworkDrives())
    , m_scanRemovable(namedScanConfig.getScanRemovableDrives())
{
    auto excludePaths = namedScanConfig.getExcludePaths();
    m_excludePaths.reserve(excludePaths.size());
    for (const auto& item : excludePaths)
    {
        m_excludePaths.emplace_back(item);
    }
}

NamedScanConfig avscanner::avscannerimpl::configFromFile(const std::string& configPath)
{
    datatypes::AutoFd fd(open(configPath.c_str(), O_RDONLY));
    if (!fd.valid())
    {
        throw std::runtime_error("Failed to open config");
    }

    ::capnp::StreamFdMessageReader message(fd);

    NamedScanConfig config(message.getRoot<Sophos::ssplav::NamedScan>());
    fd.close();

    return config;
}
