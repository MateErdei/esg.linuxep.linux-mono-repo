//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/sophos_filesystem.h"

#include <mutex>

namespace fs = sophos_filesystem;

namespace sophos_on_access_process::fanotifyhandler
{
    enum OnaccessStatus
    {
        HEALTHY = 0,
        UNHEALTHY = 1,
        INACTIVE = 2
    };

    class OnaccessStatusFile
    {
    public:
        OnaccessStatusFile();

        void setStatus(OnaccessStatus status);

    private:
        void writeStatusFile();
        OnaccessStatus m_status = INACTIVE;
        fs::path m_statusFilePath;
        mutable std::mutex m_lock;
    };
}