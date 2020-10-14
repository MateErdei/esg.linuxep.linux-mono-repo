/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NamedScanRunner.h"

#include "BaseFileWalkCallbacks.h"
#include "PathUtils.h"
#include "ScanCallbackImpl.h"
#include "ScanClient.h"

#include "avscanner/mountinfoimpl/Mounts.h"

#include <capnp/message.h>
#include <common/StringUtils.h>
#include <filewalker/FileWalker.h>

#include <fstream>
#include <set>
#include <common/AbortScanException.h>

using namespace avscanner::avscannerimpl;

NamedScanRunner::NamedScanRunner(const std::string& configPath)
    : m_config(configFromFile(configPath))
    , m_logger(m_config.m_scanName)
{
}

NamedScanRunner::NamedScanRunner(const Sophos::ssplav::NamedScan::Reader& namedScanConfig)
    : m_config(namedScanConfig)
    , m_logger(m_config.m_scanName)
{
}

namespace
{
    class CallbackImpl : public BaseFileWalkCallbacks
    {
    public:
        explicit CallbackImpl(
                ScanClient scanner,
                std::vector<fs::path> mountExclusions,
                NamedScanConfig& config,
                avscanner::mountinfo::IMountPointSharedVector allMountPoints
                )
                : BaseFileWalkCallbacks(std::move(scanner))
                , m_config(config)
                , m_allMountPoints(std::move(allMountPoints))
        {
            m_userDefinedExclusions = m_config.m_excludePaths;

            // These should always be the same because we scan all mount points on a Named Scan, but not on a Command Line Scan
            m_mountExclusions = move(mountExclusions);
            for (const auto& mountExclusion: m_mountExclusions)
            {
                m_currentExclusions.emplace_back(mountExclusion);
            }
        }

        void logScanningLine(std::string escapedPath) override
        {
            LOGDEBUG("Scanning " << escapedPath);
        }

        void genericFailure(const std::exception& e, std::string escapedPath) override
        {
            LOGERROR("Failed to scan" << escapedPath << " [" << e.what() << "]");
            m_returnCode = E_GENERIC_FAILURE;
        }

    private:
        NamedScanConfig& m_config;
        avscanner::mountinfo::IMountPointSharedVector m_allMountPoints;
    };
}

avscanner::mountinfo::IMountPointSharedVector NamedScanRunner::getIncludedMountpoints(
    const avscanner::mountinfo::IMountPointSharedVector& allMountpoints)
{
    avscanner::mountinfo::IMountPointSharedVector includedMountpoints;
    for (const auto& mp : allMountpoints)
    {
        if ((mp->isHardDisc() && m_config.m_scanHardDisc) ||
            (mp->isNetwork() && m_config.m_scanNetwork) ||
            (mp->isOptical() && m_config.m_scanOptical) ||
            (mp->isRemovable() && m_config.m_scanRemovable))
        {
            includedMountpoints.push_back(mp);
        }
        else if (mp->isSpecial() )
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " is system and will be excluded from the scan");
        }
        else
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " has been excluded from the scan");
        }
    }
    return includedMountpoints;
}

int NamedScanRunner::run()
{
    // work out which filesystems are included based of config and mount information
    auto mountInfo = getMountInfo();
    auto allMountpoints = mountInfo->mountPoints();
    LOGDEBUG("Found "<< allMountpoints.size() << " mount points");
    auto includedMountpoints = getIncludedMountpoints(allMountpoints);

    std::vector<fs::path> excludedMountPoints;
    excludedMountPoints.reserve(allMountpoints.size());
    for (const auto& mp : allMountpoints)
    {
        if (mp->isSpecial())
        {
            excludedMountPoints.emplace_back(mp->mountPoint());
            LOGINFO("Excluding mount point: " << mp->mountPoint());
        }
    }

    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();

    ScanClient scanner(*getSocket(), scanCallbacks, m_config.m_scanArchives, E_SCAN_TYPE_SCHEDULED);
    CallbackImpl callbacks(std::move(scanner), excludedMountPoints, m_config, allMountpoints);

    std::set<std::string> mountsScanned;

    scanCallbacks->scanStarted();

    // for each select included mount point call filewalker for that mount point
    for (auto & mp : includedMountpoints)
    {
        std::string mountpointToScan = mp->mountPoint();

        if(mountsScanned.find(mountpointToScan) != mountsScanned.end())
        {
            LOGINFO("Skipping duplicate mount point: " << mountpointToScan);
            continue;
        }

        LOGINFO("Attempting to scan: " << mountpointToScan);
        try
        {
            mountsScanned.insert(mountpointToScan);
            filewalker::walk(mountpointToScan, callbacks);
        }
        catch (sophos_filesystem::filesystem_error& e)
        {
            LOGERROR("Failed to completely scan " << mountpointToScan << " due to an error: " << e.what());
            m_returnCode = e.code().value();
        }

        // we want virus found to override any other return code
        if (scanCallbacks->returnCode() == E_VIRUS_FOUND)
        {
            m_returnCode = E_VIRUS_FOUND;
        }
    }

    if (m_returnCode != E_CLEAN || m_returnCode != E_VIRUS_FOUND)
    {
        LOGERROR("Failed to scan one or more files due to an error");
    }

    scanCallbacks->logSummary();

    return m_returnCode;
}

NamedScanConfig& NamedScanRunner::getConfig()
{
    return m_config;
}
