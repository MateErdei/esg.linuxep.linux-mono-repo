/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace ManagementAgent
{
    namespace StatusCache
    {
        class IStatusCache
        {

        public:
            virtual ~IStatusCache() = default;
            /**
             *
             * @param appid
             * @param statusForComparison
             * @return True if the status has changed
             */
            virtual bool statusChanged(const std::string& appid, const std::string& statusForComparison) = 0;
            /**
             * @brief Loads in any cached statuses from the disk
             */
            virtual void loadCacheFromDisk() = 0;
        };


    }
}



