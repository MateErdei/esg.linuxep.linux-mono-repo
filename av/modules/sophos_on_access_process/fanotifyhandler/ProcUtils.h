// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

// Std C++
#include <string>
// C
#include <sys/types.h>

namespace sophos_on_access_process::fanotifyhandler
{
    std::string get_executable_path_from_pid(pid_t pid);

    bool startswith(const std::string& buffer, const std::string& target);
}
