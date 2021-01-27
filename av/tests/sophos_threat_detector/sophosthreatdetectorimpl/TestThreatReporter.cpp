/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatReporter.h"

#include <gtest/gtest.h>


TEST(TestThreatReporter, testConstruction) // NOLINT
{
    sspl::sophosthreatdetectorimpl::ThreatReporter foo("/bar");
}
