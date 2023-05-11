// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ProcUtils.h"
// Component
#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace sophos_on_access_process::fanotifyhandler
{
    bool startswith(const std::string& buffer, const std::string& target)
    {
        return buffer.rfind(target, 0) == 0;
    }
}