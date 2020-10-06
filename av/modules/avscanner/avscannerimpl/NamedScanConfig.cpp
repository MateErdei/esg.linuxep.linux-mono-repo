/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NamedScanConfig.h"

#define AUTO_FD_IMPLICIT_INT
#include <datatypes/AutoFd.h>
#include <datatypes/sophos_filesystem.h>

#include <capnp/serialize.h>

#include <fstream>

#include <fcntl.h>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

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
    if (fs::is_directory(configPath))
    {
        throw std::runtime_error("Aborting: Config path must not be directory");
    }

    datatypes::AutoFd fd(open(configPath.c_str(), O_RDONLY));
    if (!fd.valid())
    {
        throw std::runtime_error("Failed to open config");
    }

    Sophos::ssplav::NamedScan::Reader messageReader;
    try
    {
        ::capnp::StreamFdMessageReader message(fd);
        messageReader = message.getRoot<Sophos::ssplav::NamedScan>();
        fd.close();
    }
    catch (const kj::Exception& e)
    {
        fd.close();
        throw std::runtime_error("Aborting: Config file cannot be parsed");
    }

    return NamedScanConfig(messageReader);
}
