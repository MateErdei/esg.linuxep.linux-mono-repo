/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "common/InotifyFD.h"

#include <sys/inotify.h>

namespace fs = sophos_filesystem;

namespace plugin::manager::scanprocessmonitor
{
    class InotifyFD : public common::InotifyFD
    {
    public:
        explicit InotifyFD(const fs::path& directory);
    };
}