/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <map>
namespace JournalerCommon
{

    enum class EventType
    {
        THREAT_EVENT
    };

    static const std::map<std::string, EventType> EventTypeMap {{"threatEvents", EventType::THREAT_EVENT}};

    struct Event
    {
        EventType type;
        std::string data;
    };
} // namespace JournalerCommon
