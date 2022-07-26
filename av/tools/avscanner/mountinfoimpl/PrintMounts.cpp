/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <iostream>

using namespace mount_monitor::mountinfoimpl;

int main()
{
    static Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    auto pathsFactory = std::make_shared<SystemPathsFactory>();
    auto mounts = std::make_shared<Mounts>(pathsFactory->createSystemPaths());

    for (const auto& m : mounts->mountPoints())
    {
        std::cout << m->device() << " on " << m->mountPoint() << " type " << m->filesystemType() << '\n';
    }
    return 0;
}
