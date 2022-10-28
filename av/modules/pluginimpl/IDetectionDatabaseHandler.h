// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "scan_messages/ThreatDetected.h"


class IDetectionDatabaseHandler
{
public:
    virtual ~IDetectionDatabaseHandler() = default;
    virtual void updateThreatDatabase(const scan_messages::ThreatDetected&) = 0;
};

