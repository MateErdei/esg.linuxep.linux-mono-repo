/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NamedScanRunner.h"

#include "BaseFileWalkCallbacks.h"
#include "Mounts.h"
#include "PathUtils.h"
#include "ScanClient.h"

#include <filewalker/FileWalker.h>

#include <capnp/message.h>

#include <fstream>
#include <common/StringUtils.h>


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
    class ScanCallbackImpl : public IScanCallbacks
    {
    public:
        void cleanFile(const path&) override
        {}
        void infectedFile(const path& p, const std::string& threatName) override
        {
            std::string escapedPath(p);
            common::escapeControlCharacters(escapedPath);
            LOGWARN("Found    \"" << escapedPath << "\" is infected with " << threatName);
            m_returnCode = E_VIRUS_FOUND;
        }

        [[nodiscard]] int returnCode() const
        {
            return m_returnCode;
        }

    private:
        int m_returnCode = E_CLEAN;
    };

    class CallbackImpl : public BaseFileWalkCallbacks
    {
    public:
        explicit CallbackImpl(
                ScanClient scanner,
                std::vector<fs::path> mountExclusions,
                NamedScanConfig& config,
                std::vector<std::shared_ptr<IMountPoint>> allMountPoints
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
                        LOGINFO("Symlink to file on excluded mount point: " << e);
                        return;
                    }
                }
            }

            for (const auto& exclusion : m_config.m_excludePaths)
            {
                if (exclusion.appliesToPath(p))
                {
                    return;
                }
            }
            try
            {
                m_scanner.scan(p);
            }
            catch (const std::exception& e)
            {
                LOGERROR("Scanning    " << p << " [" << e.what() << "] failed");
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
            for (const auto& exclusion : m_config.m_excludePaths)
            {
                if (exclusion.appliesToPath(p) && exclusion.type() != FILENAME)
                {
                    return false;
                }
            }
            return true;
        }

    private:
        std::vector<fs::path> m_mountExclusions;
        NamedScanConfig& m_config;
        std::vector<std::shared_ptr<IMountPoint>> m_allMountPoints;
    };
}

std::vector<std::shared_ptr<IMountPoint>> NamedScanRunner::getIncludedMountpoints(const std::vector<std::shared_ptr<IMountPoint>>& allMountpoints)
{
    std::vector<std::shared_ptr<IMountPoint>> includedMountpoints;
    for (auto & mp : allMountpoints)
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
    std::shared_ptr<IMountInfo> mountInfo = getMountInfo();
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    LOGINFO("Found    "<< allMountpoints.size() << " mount points");
    std::vector<std::shared_ptr<IMountPoint>> includedMountpoints = getIncludedMountpoints(allMountpoints);

    std::vector<fs::path> excludedMountPoints;
    excludedMountPoints.reserve(allMountpoints.size());
    for (const auto& mp : allMountpoints)
    {
        if (mp->isSpecial())
        {
            excludedMountPoints.emplace_back(mp->mountPoint());
        }
    }

    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();

    ScanClient scanner(*getSocket(), scanCallbacks, m_config.m_scanArchives, E_SCAN_TYPE_SCHEDULED);
    CallbackImpl callbacks(std::move(scanner), excludedMountPoints, m_config, allMountpoints);

    // for each select included mount point call filewalker for that mount point
    for (auto & mp : includedMountpoints)
    {
        std::string mountpointToScan = mp->mountPoint();
        LOGINFO("Scanning    mount point: " << mountpointToScan);
        try
        {
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
