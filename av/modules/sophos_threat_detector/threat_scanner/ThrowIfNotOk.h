// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ThreatScannerException.h"

#include "common/FailedToInitializeSusiException.h"

#include "Susi.h"

#include <stdexcept>
#include <string>

namespace threat_scanner
{
    /**
     * Checks if susi result is ok and if not throws runtime exception
     * @param res
     * @param message
     */
    inline void throwIfNotOk(SusiResult res, const std::string& message)
    {
        if (res != SUSI_S_OK)
        {
            throw ThreatScannerException(LOCATION, message);
        }
    }

    /**
     * Checks if susi result is OK and if not throws failed to initialize susi exception
     * @param res
     * @param exception
     */
    inline void throwFailedToInitIfNotOk(SusiResult res, const std::string& message)
    {
        if (res != SUSI_S_OK)
        {
            throw FailedToInitializeSusiException(LOCATION, message);
        }
    }
}
