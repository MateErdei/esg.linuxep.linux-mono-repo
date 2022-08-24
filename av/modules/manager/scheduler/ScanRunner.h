//Copyright 2020, Sophos Limited.  All rights reserved.

#pragma once

// Module
#include "ScheduledScanConfiguration.h"
// Product
#include "datatypes/sophos_filesystem.h"
// Base
#include <Common/Threads/AbstractThread.h>

namespace manager::scheduler
{
    class ScanRunner : public Common::Threads::AbstractThread
    {
    public:
        explicit ScanRunner(std::string name, std::string scan);

        [[nodiscard]] bool scanCompleted() const
        {
            return m_scanCompleted;
        }
    private:
        void run() override;

        std::string m_name;
        std::string m_configFilename;
        std::string m_scan;
        sophos_filesystem::path m_pluginInstall;
        sophos_filesystem::path m_scanExecutable;
        bool m_scanCompleted;

    };
}



