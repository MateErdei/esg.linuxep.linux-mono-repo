/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>
#include <modules/pluginimpl/Logger.h>

#include "modules/manager/scanprocessmonitor/ScanProcessMonitor.h"

using namespace plugin::manager::scanprocessmonitor;
using ScanProcessMonitorPtr = std::unique_ptr<ScanProcessMonitor>;

TEST(TestScanProcessMonitor, constructionWithArg) // NOLINT
{
    auto m = std::make_unique<ScanProcessMonitor>("/bin/bash");
}
