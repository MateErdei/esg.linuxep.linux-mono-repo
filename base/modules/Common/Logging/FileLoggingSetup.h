/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <string>

namespace Common
{
    namespace Logging
    {
        class FileLoggingSetup
        {
        public:
            explicit FileLoggingSetup(const std::string& logbase, bool lowpriv = false);
            ~FileLoggingSetup();
            static void setupFileLogging(const std::string& logbase, bool lowpriv = false);
            static void setupFileLoggingWithPath(
                    const std::string& logfilepath
                    );
        };
    }
}


