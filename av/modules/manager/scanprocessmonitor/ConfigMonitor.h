/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Threads/NotifyPipe.h"
#include "Common/Threads/AbstractThread.h"

#include <map>

namespace plugin::manager::scanprocessmonitor
{
    class ConfigMonitor : public Common::Threads::AbstractThread
    {
    public:
        explicit ConfigMonitor(Common::Threads::NotifyPipe& pipe,
                               std::string base="/etc");

    private:
        using contentMap_t = std::map<std::string, std::string>;
        void run() override;

        std::string getContents(const std::string& basename);
        contentMap_t getContentsMap();

        Common::Threads::NotifyPipe& m_configChangedPipe;
        std::string m_base;

    };
}
