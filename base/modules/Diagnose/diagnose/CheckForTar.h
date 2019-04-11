/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <string>

namespace diagnose
{
    class CheckForTar
    {
    public:
        /**
         * Check if tar is available on PATH
         * @param PATH
         * @return True if tar is available.
         */
        static bool isTarAvailable(const std::string& PATH);

        /**
         * Check if tar is available on environment variable $PATH
         * @return True if tar is available.
         */
        static bool isTarAvailable();
    };
}


