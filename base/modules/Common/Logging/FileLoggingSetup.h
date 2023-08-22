// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/FileSystem/IFilePermissions.h"

#include <string>

namespace Common::Logging
{
    class FileLoggingSetup
    {
    public:
        explicit FileLoggingSetup(const std::string& logbase, bool lowpriv);
        explicit FileLoggingSetup(const Path& logdir, const std::string& logbase);
        ~FileLoggingSetup();
        static void setupFileLogging(const std::string& logbase, bool lowpriv = false);
        static void setupFileLoggingWithPath(const std::string& logfilepath);
    };
} // namespace Common::Logging
