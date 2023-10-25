// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "PolicyParseException.h"

#include "Common/OSUtilities/IIPUtils.h"

#include <chrono>
#include <string>

namespace Common::Policy {
    template<typename IpCollection>
    std::string ipCollectionToString(const IpCollection &ipCollection);

    std::string ipToString(const Common::OSUtilities::IPs &ips);

    std::string reportToString(const Common::OSUtilities::SortServersReport &report);
}