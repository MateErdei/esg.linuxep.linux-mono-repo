// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

// Module
#include "ScheduledScanConfiguration.h"
#include "IScanComplete.h"

// Product
#include "datatypes/sophos_filesystem.h"
// Base
#include "Common/Threads/AbstractThread.h"

namespace manager::scheduler
{
    std::string generateScanCompleteXml(const std::string& name);
    std::string generateTimeStamp();

    class ScanRunner : public Common::Threads::AbstractThread
    {
    public:
        explicit ScanRunner(std::string name, std::string scan, IScanComplete& completionNotifier);

        [[nodiscard]] bool scanCompleted() const
        {
            return m_scanCompleted;
        }
    private:
        void run() override;

        IScanComplete& m_completionNotifier;
        std::string m_name;
        std::string m_configFilename;
        std::string m_scan;
        sophos_filesystem::path m_pluginInstall;
        sophos_filesystem::path m_scanExecutable;
        bool m_scanCompleted;

    };
}



