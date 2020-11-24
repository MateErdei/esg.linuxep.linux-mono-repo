/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <filewalker/IFileWalkCallbacks.h>
#include "ScanClient.h"
#include "PathUtils.h"

namespace avscanner::avscannerimpl
{
    enum E_ERROR_CODES: int
    {
        E_CLEAN = 0,
        E_GENERIC_FAILURE = 1,
        E_VIRUS_FOUND = 69
    };

    class BaseFileWalkCallbacks : public filewalker::IFileWalkCallbacks
    {
    protected:
        explicit BaseFileWalkCallbacks(ScanClient scanner);
        virtual void logScanningLine(std::string escapedPath) = 0;
        virtual void genericFailure(const std::exception& e, std::string escapedPath) = 0;
        ScanClient m_scanner;
        std::vector<fs::path>   m_mountExclusions;
        // m_currentExclusions are the exclusions that are going to be relevant to the specific scan currently running
        std::vector<Exclusion>  m_currentExclusions;
        std::vector<Exclusion>  m_userDefinedExclusions;
        int m_returnCode = E_CLEAN;

    public:
        BaseFileWalkCallbacks(const BaseFileWalkCallbacks&) = delete;
        BaseFileWalkCallbacks(BaseFileWalkCallbacks&&) = delete;
        virtual ~BaseFileWalkCallbacks() = default;

        void processFile(const fs::path& path, bool symlinkTarget) override;
        bool includeDirectory(const sophos_filesystem::path& path) override;
        bool userDefinedExclusionCheck(const sophos_filesystem::path& path, bool isSymlink) override;

        [[nodiscard]] int returnCode() const
        {
            return m_returnCode;
        }
    };
}