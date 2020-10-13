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
                , m_mountExclusions(std::move(mountExclusions))
                , m_config(config)
                , m_allMountPoints(std::move(allMountPoints))
        {}

        void processFile(const sophos_filesystem::path& p, bool symlinkTarget) override
        {
            if (symlinkTarget)
            {
                for (const auto& e : m_mountExclusions)
                {
                    if (PathUtils::startswith(p, e))
                    {
                        LOGINFO("Skipping the scanning of symlink target (" << p << ") which is on excluded mount point: " << e);
                        return;
                    }
                }
            }

            std::string escapedPath(p);
            common::escapeControlCharacters(escapedPath);

            for (const auto& exclusion : m_config.m_excludePaths)
            {
                if (exclusion.appliesToPath(p))
                {
                    LOGINFO("Excluding file: " << escapedPath);
                    return;
                }
            }

            LOGDEBUG("Scanning " << escapedPath);

            try
            {
                m_scanner.scan(p, symlinkTarget);
            }
            catch (const std::exception& e)
            {
                LOGERROR("Failed to scan" << escapedPath << " [" << e.what() << "]");
                m_returnCode = E_GENERIC_FAILURE;
            }
        }

        bool includeDirectory(const sophos_filesystem::path& p) override
        {
            for (const auto& mp : m_allMountPoints)
            {
                if (!PathUtils::longer(p, mp->mountPoint()) &&
                    PathUtils::startswith(p, mp->mountPoint()))
                {
                    return false;
                }
            }

            return !cmdExclusionCheck(p);
        }

        bool cmdExclusionCheck(const sophos_filesystem::path& p) override
        {
            for (const auto& exclusion : m_config.m_excludePaths)
            {
                if (exclusion.appliesToPath(PathUtils::appendForwardSlashToPath(p), true))
                {
                    LOGINFO("Excluding directory: " << PathUtils::appendForwardSlashToPath(p));
                    return true;
                }
            }
            return false;
        }

    private:
        std::vector<fs::path> m_mountExclusions;
        NamedScanConfig& m_config;
        avscanner::mountinfo::IMountPointSharedVector m_allMountPoints;
    };
}

avscanner::mountinfo::IMountPointSharedVector NamedScanRunner::getIncludedMountpoints(
    const avscanner::mountinfo::IMountPointSharedVector& allMountpoints)
{
    avscanner::mountinfo::IMountPointSharedVector includedMountpoints;
    for (const auto & mp : allMountpoints)
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
            m_returnCode = e.code().value();
        }

        // we want virus found to override any other return code
        if (scanCallbacks->returnCode() == E_VIRUS_FOUND)
        {
            m_returnCode = E_VIRUS_FOUND;
        }
    }

    return m_returnCode;
}

NamedScanConfig& NamedScanRunner::getConfig()
{
    return m_config;
}
