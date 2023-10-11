// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "BaseRunner.h"
#include "Logger.h"
#include "Options.h"
#include "ScanCallbackImpl.h"

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class CommandLineScanRunner : public BaseRunner
    {
    public:
        explicit CommandLineScanRunner(const Options& options);
        int run() override;

    private:
        const Options& options_;
        std::vector<std::string> m_paths;
        std::vector<std::string> m_exclusions;
        Logger m_logger;

        bool m_archiveScanning = false;
        bool m_imageScanning = false;
        bool m_followSymlinks = false;
        bool m_detectPUAs = true;
    };
}