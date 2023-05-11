// Copyright 2023 Sophos All rights reserved.
#include "ExecutablePathCache.h"

// Component
#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace sophos_on_access_process::fanotifyhandler
{
    std::string ExecutablePathCache::get_executable_path_from_pid(pid_t pid)
    {
        auto now = clock_t::now();
        auto age = now - cache_time_;
        if (age > cache_lifetime_)
        {
            cache_.clear();
            cache_time_ = now;
        }
        else
        {
            auto cached = cache_.find(pid);

            if (cached != cache_.end())
            {
                return cached->second;
            }
        }

        auto path = get_executable_path_from_pid_uncached(pid);
        cache_[pid] = path;
        return path;
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
