//
// Created by pair on 11/02/20.
//

#include "NamedScanConfig.h"

#include <fstream>

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <fcntl.h>
#include <unistd.h>

using namespace avscanner::avscannerimpl;

NamedScanConfig::NamedScanConfig(const Sophos::ssplav::NamedScan::Reader& namedScanConfig)
    : m_scanHardDisc(true)
    , m_scanOptical(true)
    , m_scanNetwork(true)
    , m_scanRemovable(true)
{
    m_scanName = namedScanConfig.getName();

    auto excludePaths = namedScanConfig.getExcludePaths();
    m_excludePaths.reserve(excludePaths.size());
    for (const auto& item : excludePaths)
    {
        m_excludePaths.emplace_back(item);
    }
}

NamedScanConfig avscanner::avscannerimpl::configFromFile(const std::string& configPath)
{
    int fd = open(configPath.c_str(), O_RDONLY);
    if (fd < 0)
    {
        throw std::runtime_error("Failed to open config");
    }

    ::capnp::StreamFdMessageReader message(fd);

    NamedScanConfig config(message.getRoot<Sophos::ssplav::NamedScan>());
    close(fd);

    return config;
}
