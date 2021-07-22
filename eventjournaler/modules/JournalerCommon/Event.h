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

    // Journal JSON content has two fields, subType and data, currently subType is not used by anything.
    static const std::map<EventType, std::string> EventTypeToJournalJsonSubtypeMap {{EventType::THREAT_EVENT, "LinuxAVThreat"}};

    // Convert between the zmq subject strings to a type of event.
    static const std::map<std::string, EventType> PubSubSubjectToEventTypeMap {{"threatEvents", EventType::THREAT_EVENT}};

    struct Event
    {
        EventType type;
        std::string data;
    };

} // namespace JournalerCommon
