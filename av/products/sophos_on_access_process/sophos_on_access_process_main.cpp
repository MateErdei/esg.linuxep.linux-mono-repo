// Copyright 2022, Sophos Limited.  All rights reserved.

#include "LogSetup.h"

#include "sophos_on_access_process/soapd_bootstrap/SoapdBootstrap.h"

int main()
{
    LogSetup logging;
    return sophos_on_access_process::soapd_bootstrap::SoapdBootstrap::runSoapd();
}
