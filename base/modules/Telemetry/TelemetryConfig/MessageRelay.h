/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Proxy.h"
#include <string>

namespace Telemetry::TelemetryConfig
{
    class MessageRelay: public Proxy
    {
    public:
        std::string m_certificatePath;

        bool operator==(const MessageRelay& rhs) const;
        bool operator!=(const MessageRelay& rhs) const;
    };
}