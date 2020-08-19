/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Susi.h>

namespace threat_scanner
{
    void susiLogCallback(void* token, SusiLogLevel level, const char* message);
    void fallbackSusiLogCallback(void* token, SusiLogLevel level, const char* message);
}
