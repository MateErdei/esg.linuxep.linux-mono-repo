/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigMonitor.h"

using namespace plugin::manager::scanprocessmonitor;

ConfigMonitor::ConfigMonitor(Common::Threads::NotifyPipe& pipe)
    : m_pipe(pipe)
{
}

void ConfigMonitor::run()
{
    announceThreadStarted();
}
