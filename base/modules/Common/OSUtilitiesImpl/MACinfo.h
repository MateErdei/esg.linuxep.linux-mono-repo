/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <array>
#include <string>
#include <vector>

namespace Common::OSUtilitiesImpl
{
    using MACType = std::array<unsigned char, 6>;
    std::string stringfyMAC(const MACType& macAddress);
    std::vector<std::string> sortedSystemMACs();
} // namespace Common::OSUtilitiesImpl
