// Copyright 2022, Sophos Limited.  All rights reserved.

#include "../common/config.h"
#include "safestore/Main.h"

#include "Common/Logging/PluginLoggingSetup.h"

int main()
{
    Common::Logging::PluginLoggingSetup setupFileLoggingWithPath(PLUGIN_NAME, "safestore");

    // TODO appConfig

    return safestore::Main::run();
}
