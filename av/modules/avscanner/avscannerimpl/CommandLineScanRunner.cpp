/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommandLineScanRunner.h"

#include "BaseFileWalkCallbacks.h"
#include "Options.h"
#include "PathUtils.h"
#include "ScanCallbackImpl.h"
#include "ScanClient.h"

#include "avscanner/mountinfoimpl/Mounts.h"
#include "common/AbortScanException.h"
#include "filewalker/FileWalker.h"

#include <common/StringUtils.h>

#include <exception>
#include <memory>
#include <utility>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

namespace
{
    class CallbackImpl : public BaseFileWalkCallbacks
    {
    public:
        explicit CallbackImpl(
                ScanClient scanner,
                std::vector<fs::path> mountExclusions,
                std::vector<Exclusion> cmdExclusions
        )
                : BaseFileWalkCallbacks(std::move(scanner))
                , m_mountExclusions(std::move(mountExclusions))
                , m_cmdExclusions(std::move(cmdExclusions))
        {
            m_currentExclusions.reserve(m_mountExclusions.size());
        }

        void processFile(const fs::path& p, bool symlinkTarget) override
        {
            if (symlinkTarget)
            {
                fs::path symlinkTargetPath = p;
                if (fs::is_symlink(fs::symlink_status(p)))
                {
                    symlinkTargetPath = fs::read_symlink(p);
                }
                for (const auto& e : m_mountExclusions)
                {

                    if (PathUtils::startswith(symlinkTargetPath, e))
                    {
                        LOGINFO("Skipping the scanning of symlink target (" << symlinkTargetPath << ") which is on excluded mount point: " << e);
                        return;
                    }
                }
            }

            std::string escapedPath(p);
            common::escapeControlCharacters(escapedPath);

            for (const auto& exclusion : m_cmdExclusions)
            {
                if (exclusion.appliesToPath(p))
                {
                    LOGINFO("Excluding file: " << p);
                    return;
                }
            }

            LOGINFO("Scanning " << escapedPath);

            try
            {
                m_scanner.scan(p, symlinkTarget);
            }
            catch (const std::exception& e)
            {
                m_returnCode = E_GENERIC_FAILURE;
                throw AbortScanException(e.what());
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

            for (const auto& exclusion : m_cmdExclusions)
            {
                if (exclusion.appliesToPath(p / "/", true))
                {
                    LOGINFO("Excluding folder: " << p);
                    return false;
                }
            }

            return true;
        }

        bool excludeDirectory(const sophos_filesystem::path& p) override
        {
            for (const auto& exclusion : m_cmdExclusions)
            {
                if (exclusion.appliesToPath(p, true))
                {
                    LOGINFO("Excluding folder: " << p);
                    return true;
                }
            }

            return false;
        }

        void setCurrentInclude(const fs::path& inclusionPath)
        {
            m_currentExclusions.clear();
            for (const auto& e : m_mountExclusions)
            {
                if (PathUtils::longer(e, inclusionPath) &&
                    PathUtils::startswith(e, inclusionPath))
                {
                    m_currentExclusions.emplace_back(e);
                }
            }
        }

    private:
        std::vector<fs::path> m_mountExclusions;
        std::vector<fs::path> m_currentExclusions;
        std::vector<Exclusion> m_cmdExclusions;
    };
}

CommandLineScanRunner::CommandLineScanRunner(const Options& options)
        : m_paths(options.paths())
        , m_exclusions(options.exclusions())
        , m_archiveScanning(options.archiveScanning())
        , m_logger(options.logFile(), true)
{
}

int CommandLineScanRunner::run()
{
    if (m_paths.empty())
    {
        LOGWARN("Missing a file path from the command line arguments.");
        LOGWARN(Options::getHelp());
        return E_GENERIC_FAILURE;
    }

    std::string printArchiveScanning = m_archiveScanning?"yes":"no";
    LOGINFO("Archive scanning enabled: " << printArchiveScanning);

    // evaluate mount information
    auto mountInfo = getMountInfo();
    auto allMountpoints = mountInfo->mountPoints();

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

    std::vector<Exclusion> cmdExclusions;
    cmdExclusions.reserve(m_exclusions.size());

    std::ostringstream oss;

    for (auto& exclusion : m_exclusions)
    {
        if( exclusion.at(0) == '.'
            or exclusion.find("/./") != std::string::npos
            or exclusion.find("/../") != std::string::npos)
        {
                exclusion = fs::canonical(exclusion);
                if(fs::is_directory(exclusion))
                {
                    exclusion.append("/");
                }
        }

        oss << exclusion << ", ";
        cmdExclusions.emplace_back(exclusion);
    }

    if (!m_exclusions.empty())
    {
        LOGINFO("Exclusions: " << oss.str());
    }

    try
    {
        auto scanCallbacks = std::make_shared<ScanCallbackImpl>();
        ScanClient scanner(*getSocket(), scanCallbacks, m_archiveScanning, E_SCAN_TYPE_ON_DEMAND);
        CallbackImpl callbacks(std::move(scanner), excludedMountPoints, cmdExclusions);

        // for each select included mount point call filewalker for that mount point
        for (const auto& path : m_paths)
        {
            auto p = fs::absolute(path);
            callbacks.setCurrentInclude(p);
            filewalker::walk(p, callbacks);
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
    catch (fs::filesystem_error& e)
    {
        m_returnCode = e.code().value();
    }
    catch (const AbortScanException& e)
    {
        LOGERROR(e.what());
    }

    return m_returnCode;
}