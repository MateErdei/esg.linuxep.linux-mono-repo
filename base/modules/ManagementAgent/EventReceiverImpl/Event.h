// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>

namespace ManagementAgent::EventReceiverImpl
{
    class Event
    {
    public:
        Event(std::string appId, std::string eventXml);
        std::string appId_;
        std::string eventXml_;
    };
}
