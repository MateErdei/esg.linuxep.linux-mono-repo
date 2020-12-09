/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScanClient.h"

#include "common/PathUtils.h"
#include "common/ErrorCodes.h"
#include <filewalker/IFileWalkCallbacks.h>

namespace avscanner::avscannerimpl
{
    class BaseFileWalkCallbacks : public filewalker::IFileWalkCallbacks
    {
    protected:
        explicit BaseFileWalkCallbacks(ScanClient scanner);
        virtual void logScanningLine(std::string escapedPath) = 0;
        void genericFailure(const std::exception& e, const std::string& escapedPath);
        ScanClient m_scanner;
        std::vector<fs::path>   m_mountExclusions;
        // m_currentExclusions are the exclusions that are going to be relevant to the specific scan currently running
        std::vector<Exclusion>  m_currentExclusions;
        std::vector<Exclusion>  m_userDefinedExclusions;
        int m_returnCode = common::E_CLEAN;

    public:
        BaseFileWalkCallbacks(const BaseFileWalkCallbacks&) = delete;
        BaseFileWalkCallbacks(BaseFileWalkCallbacks&&) = delete;

        void processFile(const fs::path& path, bool symlinkTarget) override;
        bool includeDirectory(const sophos_filesystem::path& path) override;
        bool userDefinedExclusionCheck(const sophos_filesystem::path& path, bool isSymlink) override;

        bool processSymlinkExclusions(const fs::path& path);
        [[nodiscard]] int returnCode() const
        {
            return m_returnCode;
        }
    };
}