/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "sophos_threat_detector/threat_scanner/FakeSusiScannerFactory.h"

TEST(TestThreatScanner, test_construction) //NOLINT
{
    threat_scanner::FakeSusiScannerFactory factory;
    auto scanner = factory.createScanner();
    scanner.reset();
}