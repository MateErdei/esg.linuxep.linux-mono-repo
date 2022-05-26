/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/AutoFd.h"
#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace plugin::manager::scanprocessmonitor
{
    class InotifyFD
    {
    public:
        InotifyFD(fs::path directory);
        ~InotifyFD();

        int getFD();

    private:
        datatypes::AutoFd m_inotifyFD;
        int m_watchDescriptor;
    };
}