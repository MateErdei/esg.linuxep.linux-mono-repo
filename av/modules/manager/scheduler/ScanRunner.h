/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScheduledScanConfiguration.h"

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
        std::string m_scan;
        std::string m_scanExecutable;
        bool m_scanCompleted;

        std::string generateScanCompleteXml(std::string name, std::string scan);
    };
}



