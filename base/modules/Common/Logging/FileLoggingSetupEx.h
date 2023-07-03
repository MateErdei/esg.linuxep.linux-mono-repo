/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Common/FileSystem/IFilePermissions.h"

#include <string>

namespace Common
{
    namespace Logging
    {
        class FileLoggingSetupEx
        {
        public:
            explicit FileLoggingSetupEx(const std::string& logbase, bool lowpriv);
            explicit FileLoggingSetupEx(const Path& logdir, const std::string& logbase);
            ~FileLoggingSetupEx();
            static void setupFileLogging(const std::string& logbase, bool lowpriv = false);
            static void setupFileLoggingWithPath(const std::string& logfilepath, const std::string& instanceName);
        };
    } // namespace Logging
} // namespace Common
