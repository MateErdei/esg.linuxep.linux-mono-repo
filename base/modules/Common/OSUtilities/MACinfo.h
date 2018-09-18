/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <array>
#include <vector>

namespace Common
{
    namespace OSUtilities
    {
        using MACType = std::array<unsigned char, 6>;
        std::string stringfyMAC( const MACType & macAddress);
        std::vector<std::string> sortedSystemMACs();
    }
}


