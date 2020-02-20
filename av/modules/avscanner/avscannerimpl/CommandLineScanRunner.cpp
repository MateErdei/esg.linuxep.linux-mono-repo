/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommandLineScanRunner.h"
#include "ScanClient.h"
#include "Mounts.h"
#include "PathUtils.h"

#include "datatypes/Print.h"
#include "filewalker/FileWalker.h"
#include <unixsocket/ScanningClientSocket.h>

#include <memory>
#include <utility>
#include <exception>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

enum E_ERROR_CODES: int
{
    E_CLEAN = 0,
    E_GENERIC_FAILURE = 1,
    E_VIRUS_FOUND = 69
};

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
            PRINT(p << " is infected with " << threatName);
            m_returnCode = E_VIRUS_FOUND;
        }

        [[nodiscard]] int returnCode() const
        {
            return m_returnCode;
        }

    private:
        int m_returnCode = E_CLEAN;
    };

    class CallbackImpl : public filewalker::IFileWalkCallbacks
    {
    public:
        explicit CallbackImpl(
                unixsocket::IScanningClientSocket& socket,
                std::shared_ptr<IScanCallbacks> callbacks,
                std::vector<fs::path> exclusions
                )
                : m_scanner(socket, std::move(callbacks), false)
                , m_exclusions(std::move(exclusions))
        {
            m_currentExclusions.reserve(m_exclusions.size());
        }

        void processFile(const fs::path& p) override
        {
            PRINT("Scanning " << p);
            try
            {
                m_scanner.scan(p);
            } catch (const std::exception& e)
            {
                PRINT("Scanner failed to scan: " << p);
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

        [[nodiscard]] int returnCode() const
        {
            return m_returnCode;
        }

        void setCurrentInclude(const fs::path& inclusionPath)
        {
            m_currentIncludePath = inclusionPath;
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

        ScanClient m_scanner;
        std::vector<fs::path> m_exclusions;
        std::vector<fs::path> m_currentExclusions;
        int m_returnCode = E_CLEAN;
        fs::path m_currentIncludePath;
    };
}

CommandLineScanRunner::CommandLineScanRunner(std::vector<std::string> paths)
    : m_paths(std::move(paths))
{
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
    CallbackImpl callbacks(*getSocket(), scanCallbacks, excludedMountPoints);

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
