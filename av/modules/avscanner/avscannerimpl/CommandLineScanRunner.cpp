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
        {
            m_mountExclusions = std::move(mountExclusions);
            m_currentExclusions.reserve(m_mountExclusions.size());
            m_userDefinedExclusions = std::move(cmdExclusions);
        }

        void logScanningLine(std::string escapedPath) override
        {
            LOGINFO("Scanning " << escapedPath);
        }

        void genericFailure(const std::exception& e, std::string escapedPath) override
        {
            std::ostringstream errorString;
            errorString << "Failed to scan" << escapedPath << " [" << e.what() << "]";

            m_scanner.scanError(errorString);
            m_returnCode = E_GENERIC_FAILURE;
            throw AbortScanException(e.what());
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
    };
}

CommandLineScanRunner::CommandLineScanRunner(const Options& options)
        : m_paths(options.paths())
        , m_exclusions(options.exclusions())
        , m_archiveScanning(options.archiveScanning())
        , m_logger(options.logFile(), options.logLevel(), true)
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
        if (exclusion.empty())
        {
            LOGERROR("Refusing to exclude empty path");
            continue;
        }
        if( exclusion.at(0) == '.'
            or exclusion.find("/.") != std::string::npos)
        {
            if (fs::exists(exclusion))
            {
                exclusion = fs::canonical(exclusion);
                if(fs::is_directory(exclusion) && exclusion != "/")
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

    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();
    ScanClient scanner(*getSocket(), scanCallbacks, m_archiveScanning, E_SCAN_TYPE_ON_DEMAND);
    CallbackImpl callbacks(std::move(scanner), excludedMountPoints, cmdExclusions);

    scanCallbacks->scanStarted();

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

        try
        {
            auto p = fs::absolute(path);
            callbacks.setCurrentInclude(p);
            filewalker::walk(p, callbacks);
        }
        catch (fs::filesystem_error& e)
        {
            m_returnCode = e.code().value();
        }
        catch (const AbortScanException& e)
        {
            LOGERROR(e.what());
        }
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

    scanCallbacks->logSummary();

    return m_returnCode;
}