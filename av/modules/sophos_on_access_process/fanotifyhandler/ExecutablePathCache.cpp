// Copyright 2023 Sophos Limited. All rights reserved.
#include "ExecutablePathCache.h"

// Package
#include "Logger.h"

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

        std::error_code ec;                  // ec is ignored
        auto path = get_executable_path_from_pid_uncached(pid, ec);
        if (!ec)
        {
            cache_[pid] = path;
        }
        return path;
    }

    std::string ExecutablePathCache::get_executable_path_from_pid_uncached(pid_t pid, std::error_code& ec)
    {
        fs::path target = base_;
        target /= std::to_string(pid);
        target /= "exe";
        return fs::read_symlink(target, ec); // Empty path on errors
    }
}
