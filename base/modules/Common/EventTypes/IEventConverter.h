/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IEventType.h"
#include <string>
#include <memory>

namespace Common
{
    namespace EventTypes
    {
        class IEventConverter
        {
        public:
            virtual const std::pair<std::string, std::string> eventToString(const IEventType* eventType) = 0;

        };
    }
}
