/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LogSetup.h"

#include "sophos_threat_detector/sophosthreatdetectorimpl/SophosThreatDetectorMain.h"

int main()
{
    LogSetup logging;
    return sspl::sophosthreatdetectorimpl::sophos_threat_detector_main();
}
