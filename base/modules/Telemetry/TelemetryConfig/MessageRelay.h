/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Proxy.h"

#include <string>

namespace Telemetry::TelemetryConfig
{
    class MessageRelay : public Proxy
    {
    public:
        MessageRelay();

        const std::string& getId() const;
        void setId(const std::string& id);
        int getPriority() const;
        void setPriority(int priority);

        bool operator==(const MessageRelay& rhs) const;
        bool operator!=(const MessageRelay& rhs) const;

        bool isValidMessageRelay() const;

    private:
        std::string m_id;
        int m_priority;

    };
}