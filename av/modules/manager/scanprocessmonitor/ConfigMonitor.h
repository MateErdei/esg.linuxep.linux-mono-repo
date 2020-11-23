/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Threads/NotifyPipe.h>
#include "Common/Threads/AbstractThread.h"

namespace plugin::manager::scanprocessmonitor
{
    class ConfigMonitor : public Common::Threads::AbstractThread
    {
    public:
        explicit ConfigMonitor(Common::Threads::NotifyPipe& pipe,
                               std::string base="/etc");

    private:
        void run() override;

        Common::Threads::NotifyPipe& m_configChangedPipe;
        std::string m_base;

    };
}
