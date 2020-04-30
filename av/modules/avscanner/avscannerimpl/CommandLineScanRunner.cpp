/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommandLineScanRunner.h"

#include "BaseFileWalkCallbacks.h"
#include "Mounts.h"
#include "PathUtils.h"
#include "ScanClient.h"

#include "datatypes/Print.h"
#include "filewalker/FileWalker.h"
#include <common/StringUtils.h>

#include <memory>
#include <utility>
#include <exception>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

namespace
{
    class ScanCallbackImpl : public IScanCallbacks
    {
    public:
        void cleanFile(const path&) override
        {
        }

        void infectedFile(const path& p, const std::string& threatName) override
        {
            std::string escapedPath(p);
            common::escapeControlCharacters(escapedPath);
            PRINT("\"" << escapedPath << "\" is infected with " << threatName);
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
                std::vector<fs::path> exclusions
                )
                : BaseFileWalkCallbacks(std::move(scanner))
                , m_exclusions(std::move(exclusions))
        {
            m_currentExclusions.reserve(m_exclusions.size());
        }

        void processFile(const fs::path& p) override
        {
            std::string escapedPath(p);
            common::escapeControlCharacters(escapedPath);
            PRINT("Scanning " << escapedPath);
            try
            {
                m_scanner.scan(p);
            } catch (const std::exception& e)
            {
                PRINT("Scanner failed to scan: " << escapedPath << " [" << e.what() << "]");

                m_returnCode = E_GENERIC_FAILURE;
            }
        }

        bool includeDirectory(const sophos_filesystem::path& p) override
        {
            for (auto & exclusion : m_currentExclusions)
            {
                if (PathUtils::startswith(p, exclusion))
                {
                    return false;
                }
            }
            return true;
        }

        void setCurrentInclude(const fs::path& inclusionPath)
        {
            m_currentExclusions.clear();
            for (const auto& e : m_exclusions)
            {
                if (PathUtils::longer(e, inclusionPath) &&
                    PathUtils::startswith(e, inclusionPath))
                {
                    m_currentExclusions.emplace_back(e);
                }
            }
        }

    private:
        std::vector<fs::path> m_exclusions;
        std::vector<fs::path> m_currentExclusions;
    };
}

CommandLineScanRunner::CommandLineScanRunner(std::vector<std::string> paths, bool archiveScanning)
    : m_paths(std::move(paths))
    , m_archiveScanning(archiveScanning)
{
    PRINT("Scanning Options");
    PRINT("Scan archives enabled:" << archiveScanning);
}

int CommandLineScanRunner::run()
{
    // evaluate mount information
    std::shared_ptr<IMountInfo> mountInfo = getMountInfo();
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    std::vector<fs::path> excludedMountPoints;
    excludedMountPoints.reserve(allMountpoints.size());
    for (const auto& mp : allMountpoints)
    {
        if (!mp->isSpecial())
        {
            continue;
        }
        excludedMountPoints.emplace_back(mp->mountPoint());
    }

    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();
    ScanClient scanner(*getSocket(), scanCallbacks, m_archiveScanning, E_SCAN_TYPE_ON_DEMAND);
    CallbackImpl callbacks(std::move(scanner), excludedMountPoints);

    // for each select included mount point call filewalker for that mount point
    for (const auto& path : m_paths)
    {
        try
        {
            auto p = fs::absolute(path);
            callbacks.setCurrentInclude(p);
            filewalker::walk(p, callbacks);
        } catch (fs::filesystem_error& e)
        {
            m_returnCode = e.code().value();
        }

        // we want virus found to override any other return code
        if (scanCallbacks->returnCode() != E_CLEAN)
        {
            m_returnCode = scanCallbacks->returnCode();
        }
        else if (callbacks.returnCode() != E_CLEAN)
        {
            m_returnCode = callbacks.returnCode();
        }
    }

    return m_returnCode;
}
