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
        /**
         * To enable tests that need to provide some fixed timestamp.
         */
        class IFormattedTime
        {
        public:
            virtual ~IFormattedTime() = default;
            virtual std::string currentTime() const = 0;
            virtual std::string bootTime() const = 0;
        };

        class TimeUtils
        {
        public:
            static std::time_t getCurrTime() ;
            static std::string getBootTime();
            static std::time_t getBootTimeAsTimet();

            /**
             * Return timestamp formated as required by Timestamp Event
             * https://wiki.sophos.net/display/SophosCloud/EMP%3A+event-alc
             *
             * YYYYMMDD HHMMSS
             * @return timestamp formated as required above.
             */
            static std::string fromTime(std::time_t);
        };

        class FormattedTime
                : public virtual IFormattedTime
        {
        public:
            std::string currentTime() const override ;
            std::string bootTime() const override ;
        };

    }
}


