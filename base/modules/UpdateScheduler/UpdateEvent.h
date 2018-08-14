/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <vector>

namespace UpdateScheduler
{

    struct MessageInsert
    {
        MessageInsert(std::string pN, std::string err): PackageName(std::move(pN)), ErrorDetails(std::move(err)){}
        std::string PackageName;
        std::string ErrorDetails;
    };

    struct UpdateEvent
    {
        UpdateEvent():IsRelevantToSend(false), MessageNumber(0),Messages(),UpdateSource(){}
        bool IsRelevantToSend;
        int MessageNumber;
        std::vector<MessageInsert> Messages;
        std::string UpdateSource;
    };

    std::string serializeUpdateEvent( const UpdateEvent & );
}




