//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "mount_monitor/mountinfo/IMountInfo.h"

#include "datatypes/AutoFd.h"

namespace sophos_on_access_process::fanotifyhandler
{
    class FANotifyHandler
    {
        public:
            FANotifyHandler();
            ~FANotifyHandler();
            FANotifyHandler(const FANotifyHandler&) =delete;
            FANotifyHandler& operator=(const FANotifyHandler&) =delete;

            int faNotifyFd();

        private:
            datatypes::AutoFd m_fd;
    };
}

