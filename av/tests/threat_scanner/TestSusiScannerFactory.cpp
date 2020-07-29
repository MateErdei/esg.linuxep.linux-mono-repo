/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"

using namespace threat_scanner;

TEST(TestSusiScannerFactory, testConstruction) // NOLINT
{
    EXPECT_NO_THROW(SusiScannerFactory factory;);
}
