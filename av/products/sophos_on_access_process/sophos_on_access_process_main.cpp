// Copyright 2022, Sophos Limited.  All rights reserved.

#include "../common/config.h"
#include "sophos_on_access_process/soapd_bootstrap/SoapdBootstrap.h"

#include "Common/Logging/PluginLoggingSetup.h"

int main()
{
    Common::Logging::PluginLoggingSetup setupFileLoggingWithPath(PLUGIN_NAME, "soapd");
    return sophos_on_access_process::soapd_bootstrap::SoapdBootstrap::runSoapd();
}
