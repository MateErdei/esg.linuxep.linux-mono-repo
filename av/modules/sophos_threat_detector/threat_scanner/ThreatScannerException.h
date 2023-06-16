// Copyright 2023 Sophos All rights reserved.

#pragma once

#include "datatypes/AVException.h"

namespace threat_scanner
{
    class ThreatScannerException : public datatypes::AVException
    {
    public:
        using datatypes::AVException::AVException;
    };
}
