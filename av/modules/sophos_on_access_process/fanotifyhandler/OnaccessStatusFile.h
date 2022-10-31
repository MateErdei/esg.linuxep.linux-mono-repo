//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/OnaccessStatus.h"
#include "datatypes/sophos_filesystem.h"

#include <mutex>

namespace fs = sophos_filesystem;

namespace sophos_on_access_process::fanotifyhandler
{
    class OnaccessStatusFile
    {
    public:
        OnaccessStatusFile();

        void setStatus(datatypes::OnaccessStatus status);

    private:
        void writeStatusFile();
        datatypes::OnaccessStatus m_status = datatypes::OnaccessStatus::INACTIVE;
        fs::path m_statusFilePath;
        mutable std::mutex m_lock;
    };
}