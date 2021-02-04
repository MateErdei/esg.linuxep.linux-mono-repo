/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommandLineScanRunner.h"

#include "BaseFileWalkCallbacks.h"
#include "Options.h"
#include "ScanClient.h"

#include "avscanner/mountinfoimpl/Mounts.h"
#include "filewalker/FileWalker.h"

#include "common/AbortScanException.h"
#include "common/PathUtils.h"
#include <common/StringUtils.h>

#include <csignal>
#include <exception>
#include <memory>
#include <utility>

namespace avscanner::avscannerimpl
{
    namespace fs = sophos_filesystem;

    namespace
    {
        class CommandLineWalkerCallbackImpl : public BaseFileWalkCallbacks
        {
        public:
            explicit CommandLineWalkerCallbackImpl(
                std::shared_ptr<IScanClient> scanner,
                std::shared_ptr<SigIntMonitor> sigIntMonitor,
                std::vector<fs::path> mountExclusions,
                std::vector<Exclusion> cmdExclusions) :
                BaseFileWalkCallbacks(std::move(scanner))
            {
                m_sigIntMonitor = std::move(sigIntMonitor);
                m_mountExclusions = std::move(mountExclusions);
                m_currentExclusions.reserve(m_mountExclusions.size());
                m_userDefinedExclusions = std::move(cmdExclusions);
            }

            void logScanningLine(std::string escapedPath) override
            {
                LOGINFO("Scanning " << escapedPath);
            }

            void setCurrentInclude(const fs::path& inclusionPath)
            {
                m_currentExclusions.clear();
                for (const auto& e : m_mountExclusions)
                {
                    if (common::PathUtils::longer(e, inclusionPath) && common::PathUtils::startswith(e, inclusionPath))
                    {
                        m_currentExclusions.emplace_back(e);
                    }
                }
            }

            void checkIfScanAborted() override
            {
                if (m_sigIntMonitor->triggered())
                {
                    LOGDEBUG("Received SIGINT");
                    throw AbortScanException("Scan manually aborted");
                }
            }

        protected:
            std::shared_ptr<SigIntMonitor> m_sigIntMonitor;

        };
    }

    CommandLineScanRunner::CommandLineScanRunner(const Options& options) :
        m_paths(options.paths()),
        m_exclusions(options.exclusions()),
        m_logger(options.logFile(), options.logLevel(), true),
        m_archiveScanning(options.archiveScanning()),
        m_followSymlinks(options.followSymlinks())
    {
        m_sigIntMonitor = std::make_shared<SigIntMonitor>();
    }

    int CommandLineScanRunner::run()
    {
        if (m_paths.empty())
        {
            LOGWARN("Missing a file path from the command line arguments.");
            LOGWARN(Options::getHelp());
            return common::E_GENERIC_FAILURE;
        }

        std::string printArchiveScanning = m_archiveScanning ? "yes" : "no";
        LOGINFO("Archive scanning enabled: " << printArchiveScanning);

        std::string printFollowSymlink = m_followSymlinks ? "yes" : "no";
        LOGINFO("Following symlinks: " << printFollowSymlink);

        // evaluate mount information
        auto mountInfo = getMountInfo();
        auto allMountpoints = mountInfo->mountPoints();

        std::vector<fs::path> excludedMountPoints;
        excludedMountPoints.reserve(allMountpoints.size());
        for (const auto& mp : allMountpoints)
        {
            if (mp->isSpecial())
            {
                excludedMountPoints.emplace_back(mp->mountPoint() + "/");
            }
        }

        std::vector<Exclusion> cmdExclusions;
        cmdExclusions.reserve(m_exclusions.size());

        std::ostringstream oss;

        for (auto& exclusion : m_exclusions)
        {
            if (exclusion.empty())
            {
                LOGERROR("Refusing to exclude empty path");
                continue;
            }
            if (exclusion.at(0) == '.' or exclusion.find("/.") != std::string::npos)
            {
                if (fs::exists(exclusion))
                {
                    exclusion = fs::canonical(exclusion);
                    if (fs::is_directory(exclusion) && exclusion != "/")
                    {
                        exclusion.append("/");
                    }
                }
                else
                {
                    LOGERROR("Cannot canonicalize: " << exclusion);
                }
            }

            oss << exclusion << ", ";
            cmdExclusions.emplace_back(exclusion);
        }

        if (!m_exclusions.empty())
        {
            LOGINFO("Exclusions: " << oss.str());
        }

        m_scanCallbacks = std::make_shared<ScanCallbackImpl>();
        auto scanner =
            std::make_shared<ScanClient>(*getSocket(), m_scanCallbacks, m_archiveScanning, E_SCAN_TYPE_ON_DEMAND);
        CommandLineWalkerCallbackImpl commandLineWalkerCallbacks(scanner, m_sigIntMonitor, excludedMountPoints, cmdExclusions);
        filewalker::FileWalker fw(commandLineWalkerCallbacks);
        fw.followSymlinks(m_followSymlinks);

        m_scanCallbacks->scanStarted();

        // for each select included mount point call filewalker for that mount point
        for (auto& path : m_paths)
        {
            if (path.empty())
            {
                LOGERROR("Refusing to scan empty path");
                continue;
            }
            if ((path.at(0) == '.' or path.find("/.") != std::string::npos) and fs::exists(path))
            {
                path = fs::canonical(path);
            }

            auto p = fs::absolute(path);
            commandLineWalkerCallbacks.setCurrentInclude(p);

            if (!walk(fw, p, path))
            {
                // Abort scan
                break;
            }
        }

        // Override E_CLEAN with E_GENERIC_FAILURE but don't override if a different error code has already been set
        if (commandLineWalkerCallbacks.returnCode() != common::E_CLEAN && m_returnCode == common::E_CLEAN)
        {
            // returnCode() is either E_CLEAN or E_GENERIC_FAILURE
            m_returnCode = commandLineWalkerCallbacks.returnCode();
        }

        // We want virus found to override any other return code
        if (m_scanCallbacks->returnCode() == common::E_VIRUS_FOUND)
        {
            m_returnCode = common::E_VIRUS_FOUND;
        }

        if (m_returnCode != common::E_CLEAN && m_returnCode != common::E_VIRUS_FOUND)
        {
            LOGERROR("Failed to scan one or more files due to an error");
        }

        m_scanCallbacks->logSummary();

        return m_returnCode;
    }
}