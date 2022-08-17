//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"

namespace sophos_on_access_process::fanotifyhandler
{
    class FANotifyHandler
    {
        public:
            explicit FANotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);
            ~FANotifyHandler();
            FANotifyHandler(const FANotifyHandler&) =delete;
            FANotifyHandler& operator=(const FANotifyHandler&) =delete;

            int faNotifyFd();

        private:
            datatypes::AutoFd m_fd;
    };
}

