// Copyright 2022, Sophos Limited. All rights reserved.

#pragma once

#include "ScanResult.h"

#include <datatypes/AutoFd.h>

namespace threat_scanner
{
    class IUnitScanner
    {
    public:
        [[nodiscard]] virtual ScanResult scan(datatypes::AutoFd& fd, const std::string& path) = 0;
        virtual ~IUnitScanner() = default;
    };
} // namespace threat_scanner
