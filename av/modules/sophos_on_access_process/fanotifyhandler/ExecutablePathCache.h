// Copyright 2023 Sophos All rights reserved.
#pragma once

// Std C++
#include <string>
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
    };
}
