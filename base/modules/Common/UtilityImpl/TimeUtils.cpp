/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TimeUtils.h"
namespace  Common
{
    namespace UtilityImpl
    {

        std::string TimeUtils::fromTime(std::time_t time_)
        {
            if ( time_ == -1 )
            {
                return "";
            }

            char formattedTime[16];
            strftime(formattedTime, 16, "%Y%m%d %H%M%S", std::localtime(&time_));

            return formattedTime;

        }

        std::time_t TimeUtils::getCurrTime()
        {
            return std::time(nullptr);
        }

        std::string TimeUtils::getBootTime()
        {
            return std::__cxx11::string();
        }



        std::string FormatedTime::currentTime() const
        {
            return TimeUtils::fromTime( TimeUtils::getCurrTime());
        }
    }
}