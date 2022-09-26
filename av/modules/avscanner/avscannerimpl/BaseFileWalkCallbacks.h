// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IScanClient.h"

#include "common/ErrorCodes.h"
#include "common/Exclusion.h"
#include "common/PathUtils.h"

#include <filewalker/IFileWalkCallbacks.h>

namespace avscanner::avscannerimpl
{
    class BaseFileWalkCallbacks : public filewalker::IFileWalkCallbacks
    {
    protected:
        explicit BaseFileWalkCallbacks(std::shared_ptr<IScanClient> scanner);
        virtual void logScanningLine(std::string escapedPath) = 0;
        void genericFailure(const std::exception& e, const std::string& escapedPath);
        bool excludeSymlink(const fs::path& path);
        virtual void checkIfScanAborted() {};

        std::shared_ptr<IScanClient> m_scanner;
        std::vector<fs::path> m_mountExclusions;
        // m_currentExclusions are the exclusions that are going to be relevant to the specific scan currently running
        std::vector<common::Exclusion>  m_currentExclusions;
        std::vector<common::Exclusion>  m_userDefinedExclusions;
        int m_returnCode = common::E_CLEAN_SUCCESS;

    public:

        BaseFileWalkCallbacks(const BaseFileWalkCallbacks&) = delete;
        BaseFileWalkCallbacks(BaseFileWalkCallbacks&&) = delete;

        void processFile(const fs::path& path, bool symlinkTarget) override;
        bool includeDirectory(const sophos_filesystem::path& path) override;
        bool userDefinedExclusionCheck(const sophos_filesystem::path& path, bool isSymlink) override;
        void registerError(const std::ostringstream &errorString, std::error_code errorCode) override
        {
            m_scanner->scanError(errorString, errorCode);
        };

        [[nodiscard]] int returnCode() const
        {
            return m_returnCode;
        }

    };
} // namespace avscanner::avscannerimpl