// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ProcUtils.h"
// Component
#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

std::string sophos_on_access_process::fanotifyhandler::get_executable_path_from_pid(pid_t pid)
{
    fs::path target = "/proc";
    target /= std::to_string(pid);
    target /= "exe";
    std::error_code ec; // ec is ignored
    return fs::read_symlink(target, ec); // Empty path on errors
}

bool sophos_on_access_process::fanotifyhandler::startswith(const std::string& buffer, const std::string& target)
{
    return buffer.find(target) == 0;
}
