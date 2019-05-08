/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <ctime>
#include <memory>
#include <string>

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

        class ITime
        {
        public:
            virtual ~ITime() = default;
            virtual std::time_t getCurrentTime() = 0;
        };

        /**
         * This class is to be used in test only. It allows testers to redirect: TimeUtils::getCurrTime to
         * the provided mockTimer->getCurrentTime.
         * The object created will setup the time replacement and it will restore it on its destruction.
         * As such, this class is not meant to be copied or moved.
         */
        class ScopedReplaceITime
        {
        public:
            ScopedReplaceITime(std::unique_ptr<ITime> mockTimer);
            ~ScopedReplaceITime();
            ScopedReplaceITime(const ScopedReplaceITime&) = delete;
            ScopedReplaceITime& operator=(const ScopedReplaceITime) = delete;
            // not necessary to state the move operators as the constructor will not create them anyway.
        };

        class TimeUtils
        {
        public:
            static std::time_t getCurrTime();
            static std::string getBootTime();
            static std::time_t getBootTimeAsTimet();

            /**
             * Return timestamp formatted as required by Timestamp Event
             * https://wiki.sophos.net/display/SophosCloud/EMP%3A+event-alc
             *
             * YYYYMMDD HHMMSS
             * @return timestamp formatted as required above.
             */
            static std::string fromTime(std::time_t);
            static std::string fromTime(std::tm);
        };

        class FormattedTime : public virtual IFormattedTime
        {
        public:
            std::string currentTime() const override;
            std::string bootTime() const override;
        };
    } // namespace UtilityImpl
} // namespace Common
