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
        std::string PackageName;
        std::string ErrorDetails;
    };

    struct UpdateEvent
    {
        bool IsRelevantToSend;
        int MessageNumber;
        std::vector<MessageInsert> Messages;
        std::string UpdateSource;
    };

    std::string serializeUpdateEvent( const UpdateEvent & );
}




