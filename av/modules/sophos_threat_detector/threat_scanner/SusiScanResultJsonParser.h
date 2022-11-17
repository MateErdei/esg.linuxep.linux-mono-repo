// Copyright 2020-2022, Sophos Limited. All rights reserved.

#pragma once

#include "ScanResult.h"

namespace threat_scanner
{
    ScanResult parseSusiScanResultJson(const std::string& json) noexcept;
}; // namespace threat_scanner
