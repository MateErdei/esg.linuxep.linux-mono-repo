/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "WaitForUtils.h"

#include <functional>
#include <unistd.h>

namespace Common::UtilityImpl
{
    bool waitFor(double timeToWaitInSeconds, double intervalInSeconds, std::function<bool()> conditionFunction)
    {
        if (conditionFunction())
        {
            return true;
        }
        else
        {
            unsigned int intervalMicroSeconds = intervalInSeconds * 1000000;
            double maxNumberOfSleeps = timeToWaitInSeconds / intervalInSeconds;
            double sleepCounter = 0;
            while (sleepCounter++ < maxNumberOfSleeps)
            {
                if (conditionFunction())
                {
                    return true;
                }
                else
                {
                    usleep(intervalMicroSeconds);
                }
            }
        }
        return false;
    }
} // namespace Common::UtilityImpl
