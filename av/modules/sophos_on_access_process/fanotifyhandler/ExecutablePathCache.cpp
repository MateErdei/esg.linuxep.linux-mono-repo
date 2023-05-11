// Copyright 2023 Sophos All rights reserved.
#include "ExecutablePathCache.h"

// Component
#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace sophos_on_access_process::fanotifyhandler
{
    std::string ExecutablePathCache::get_executable_path_from_pid(pid_t pid)
    {
        return get_executable_path_from_pid_uncached(pid);
    }

    std::string ExecutablePathCache::get_executable_path_from_pid_uncached(pid_t pid)
    {
        fs::path target = "/proc";
        target /= std::to_string(pid);
        target /= "exe";
        std::error_code ec;                  // ec is ignored
        return fs::read_symlink(target, ec); // Empty path on errors
    }
}
