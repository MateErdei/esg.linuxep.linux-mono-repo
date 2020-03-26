/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "threat_scanner/SusiScannerFactory.h"

TEST(TestThreatScanner, test_construction) //NOLINT
{
    threat_scanner::SusiScannerFactory factory;
    auto scanner = factory.createScanner();
    scanner.reset();
}