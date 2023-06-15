// Copyright 2023 Sophos All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

namespace threat_scanner
{
    class ThreatScannerException : public Common::Exceptions::IException
    {
    public:
        using Common::Exceptions::IException::IException;
    };
}
