/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScheduledScanConfiguration.h"
#include "IScanComplete.h"

#include <Common/Threads/AbstractThread.h>

namespace manager::scheduler
{
    std::string generateScanCompleteXml(const std::string& name);

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
        std::string m_scan;
        std::string m_scanExecutable;
        bool m_scanCompleted;

    };
}



