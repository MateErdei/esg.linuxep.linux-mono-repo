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
                std::vector<fs::path> mountExclusions,
                std::vector<Exclusion> cmdExclusions) :
                BaseFileWalkCallbacks(std::move(scanner))
            {
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
        };
    }

    CommandLineScanRunner::CommandLineScanRunner(const Options& options) :
        m_paths(options.paths()),
        m_exclusions(options.exclusions()),
        m_logger(options.logFile(), options.logLevel(), true),
        m_archiveScanning(options.archiveScanning()),
        m_followSymlinks(options.followSymlinks())
    {
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

            if (common::PathUtils::isNonNormalisedPath(exclusion))
            {
                exclusion = common::PathUtils::lexicallyNormal(exclusion);

                // if we don't remove the forward slash, it will be resolved to the target path
                if (fs::exists(exclusion) && !fs::is_symlink(fs::symlink_status(common::PathUtils::removeForwardSlashFromPath(exclusion))))
                {
                    exclusion = fs::canonical(exclusion);
                }
                else
                {
                    LOGDEBUG("Will not canonicalize further: " << exclusion);
                }

                if (fs::is_directory(exclusion) && exclusion != "/")
                {
                    exclusion = common::PathUtils::appendForwardSlashToPath(exclusion);
                }
            }

            oss << exclusion << ", ";
            cmdExclusions.emplace_back(fs::path(exclusion));
        }

        if (!m_exclusions.empty())
        {
            LOGINFO("Exclusions: " << oss.str());
        }

        m_scanCallbacks = std::make_shared<ScanCallbackImpl>();
        auto scanner =
            std::make_shared<ScanClient>(*getSocket(), m_scanCallbacks, m_archiveScanning, E_SCAN_TYPE_ON_DEMAND);
        CommandLineWalkerCallbackImpl commandLineWalkerCallbacks(scanner, excludedMountPoints, cmdExclusions);
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

            auto pathString = fs::absolute(path).string();

            if (pathString.find("/.") != std::string::npos and fs::exists(pathString))
            {
                pathString = common::PathUtils::lexicallyNormal(pathString);
            }

            auto p = fs::path(pathString);
            commandLineWalkerCallbacks.setCurrentInclude(p);

            if (!walk(fw, p, path))
            {
                // Abort scan
                break;
            }
        }

        // We want virus found to override any other return code
        if (m_scanCallbacks->returnCode() == common::E_VIRUS_FOUND)
        {
            m_returnCode = common::E_VIRUS_FOUND;
        }

        // E_GENERIC_FAILURE should override E_CLEAN_SUCCESS but no other error code
        if (m_returnCode == common::E_CLEAN_SUCCESS)
        {
            // Tracks erros in filewalker is either E_CLEAN_SUCCESS or E_GENERIC_FAILURE
            m_returnCode = commandLineWalkerCallbacks.returnCode();

            if (m_returnCode == common::E_CLEAN_SUCCESS)
            {
                // Tracks errors in ScanClient
                m_returnCode = m_scanCallbacks->returnCode();
            }
        }

        bool printErrorMessage = m_returnCode != common::E_CLEAN_SUCCESS
                                && m_returnCode != common::E_VIRUS_FOUND
                                && m_returnCode != common::E_EXECUTION_MANUALLY_INTERRUPTED;
        if (printErrorMessage)
        {
            LOGERROR("Failed to scan one or more files due to an error");
        }

        m_scanCallbacks->logSummary();

        return m_returnCode;
    }
}