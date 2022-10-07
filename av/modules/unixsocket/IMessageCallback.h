// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "scan_messages/ThreatDetected.h"

#include <string>

class IMessageCallback
{
public:
    virtual ~IMessageCallback() = default;
    virtual void processMessage(scan_messages::ThreatDetected detection) = 0;
};
