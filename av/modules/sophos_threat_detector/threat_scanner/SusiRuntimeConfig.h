// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "common/ThreatDetector/SusiSettings.h"

namespace threat_scanner
{
    std::string createRuntimeConfig(
        const std::string& scannerInfo,
        const std::string& endpointId,
        const std::string& customerId,
        const std::shared_ptr<common::ThreatDetector::SusiSettings>& settings);
}
