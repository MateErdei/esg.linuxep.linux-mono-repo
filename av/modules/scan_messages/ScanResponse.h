/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace scan_messages
{
    class ScanResponse
    {
    public:
        std::string serialise();
    };
}

