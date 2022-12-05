// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ProcUtils.h"
// Component
#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace sophos_on_access_process::fanotifyhandler
{
    std::string get_executable_path_from_pid(pid_t pid)
    {
        fs::path target = "/proc";
        target /= std::to_string(pid);
        target /= "exe";
        std::error_code ec;                  // ec is ignored
        return fs::read_symlink(target, ec); // Empty path on errors
    }

    bool startswith(const std::string& buffer, const std::string& target)
    {
        return buffer.rfind(target, 0) == 0;
    }
}