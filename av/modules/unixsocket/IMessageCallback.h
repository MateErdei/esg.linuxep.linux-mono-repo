/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "scan_messages/ServerThreatDetected.h"

#include <string>

class IMessageCallback
{
public:
    virtual ~IMessageCallback() = default;
    virtual void processMessage(const scan_messages::ServerThreatDetected& detection) = 0;
};
