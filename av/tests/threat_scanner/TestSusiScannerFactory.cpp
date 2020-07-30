/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "datatypes/Print.h"

#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"

using namespace threat_scanner;

TEST(TestSusiScannerFactory, testConstruction) // NOLINT
{

    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/opt/not-sophos-spl/plugins/av");

    try
    {
        SusiScannerFactory factory;
        FAIL() << "Able to construct SusiScannerFactory";
    }
    catch (const std::exception& ex)
    {
        PRINT("Unable to construct factory" << ex.what() << '\n');
    }
}
