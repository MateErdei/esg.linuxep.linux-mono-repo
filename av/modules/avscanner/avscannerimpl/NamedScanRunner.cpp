/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NamedScanRunner.h"

#include "BaseFileWalkCallbacks.h"
#include "Mounts.h"
#include "PathUtils.h"
#include "ScanClient.h"

#include <filewalker/FileWalker.h>
#include <unixsocket/threatDetectorSocket/ScanningClientSocket.h>

#include <capnp/message.h>

#include <fstream>


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
            LOGINFO(p << " is infected with " << threatName);
            m_returnCode = E_VIRUS_FOUND;
        }

        [[nodiscard]] int returnCode() const
        {
            return m_returnCode;
        }

    private:
        int m_returnCode = E_CLEAN;
    };

    bool startwithany(const std::vector<std::string>& paths, const std::string& p)
    {
        for (auto & exclusion : paths)
        {
            if (PathUtils::startswith(p, exclusion))
            {
                return true;
            }
        }
        return false;
    }

    class CallbackImpl : public BaseFileWalkCallbacks
    {
    public:
        explicit CallbackImpl(
                ScanClient scanner,
                NamedScanConfig& config,
                std::vector<std::shared_ptr<IMountPoint>> allMountPoints
                )
                : BaseFileWalkCallbacks(std::move(scanner))
                , m_config(config)
                , m_allMountPoints(std::move(allMountPoints))
        {}

        void processFile(const sophos_filesystem::path& p) override
        {
            if (startwithany(m_config.m_excludePaths, p))
            {
                return;
            }
            try
            {
                m_scanner.scan(p);
            }
            catch (const std::exception& e)
            {
                LOGERROR("Scanner failed to scan: " << p);
                m_returnCode = E_GENERIC_FAILURE;
            }
        }

        bool includeDirectory(const sophos_filesystem::path& p) override
        {
            for (auto & mp : m_allMountPoints)
            {
                if (!PathUtils::longer(p, mp->mountPoint()) &&
                    PathUtils::startswith(p, mp->mountPoint()))
                {
                    return false;
                }
            }
            if (startwithany(m_config.m_excludePaths, p)) //NOLINT
            {
                return false;
            }
            return true;
        }

    private:
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
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " is has been excluded from the scan");
        }
    }
    return includedMountpoints;
}

static bool contains(std::vector<std::string>& paths, const std::string& p)
{
    return std::find(paths.begin(), paths.end(), p) != paths.end();
}

int NamedScanRunner::run()
{
    // work out which filesystems are included based of config and mount information
    std::shared_ptr<IMountInfo> mountInfo = getMountInfo();
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    LOGINFO("Found "<< allMountpoints.size() << " mount points");
    std::vector<std::shared_ptr<IMountPoint>> includedMountpoints = getIncludedMountpoints(allMountpoints);

    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();

    ScanClient scanner(*getSocket(), scanCallbacks, m_config);
    CallbackImpl callbacks(std::move(scanner), m_config, allMountpoints);

    // for each select included mount point call filewalker for that mount point
    for (auto & mp : includedMountpoints)
    {
        std::string mountpointToScan = mp->mountPoint();
        auto mpSlash = mountpointToScan + "/";

        if (contains(m_config.m_excludePaths, mpSlash) or startwithany(m_config.m_excludePaths, mpSlash))
        {
            LOGINFO("Excluding mount point: " << mountpointToScan);
            continue;
        }

        LOGINFO("Scanning mount point: " << mountpointToScan);
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
