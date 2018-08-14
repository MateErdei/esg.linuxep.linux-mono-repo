/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <ctime>

namespace Common
{
    namespace UtilityImpl
    {
        class TimeUtils
        {
        public:
            static std::time_t getCurrTime() ;
            static std::string getBootTime();

            /**
             * Return timestamp formated as required by Timestamp Event
             * https://wiki.sophos.net/display/SophosCloud/EMP%3A+event-alc
             *
             * YYYYMMDD HHMMSS
             * @return timestamp formated as required above.
             */
            static std::string fromTime(std::time_t);
        };
    }
}


