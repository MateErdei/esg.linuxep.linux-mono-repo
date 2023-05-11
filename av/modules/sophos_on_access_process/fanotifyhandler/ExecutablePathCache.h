// Copyright 2023 Sophos All rights reserved.
#pragma once

// Std C++
#include <chrono>
#include <string>
#include <unordered_map>
// C
#include <sys/types.h>

namespace sophos_on_access_process::fanotifyhandler
{
    class ExecutablePathCache
    {
    public:
        virtual ~ExecutablePathCache() = default;
        std::string get_executable_path_from_pid(pid_t pid);
        virtual std::string get_executable_path_from_pid_uncached(pid_t pid);
    protected:
        using cache_t = std::unordered_map<pid_t, std::string>;
        using clock_t = std::chrono::steady_clock;
        std::chrono::milliseconds cache_lifetime_ = std::chrono::seconds(1);
    private:
        cache_t cache_;
        clock_t::time_point cache_time_ = clock_t::now();
    };
}
