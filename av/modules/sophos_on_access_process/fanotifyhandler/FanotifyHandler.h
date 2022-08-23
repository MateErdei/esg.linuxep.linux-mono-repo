//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"

namespace sophos_on_access_process::fanotifyhandler
{
    class FanotifyHandler
    {
        public:
            explicit FanotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);
            ~FanotifyHandler();
            FanotifyHandler(const FanotifyHandler&) =delete;
            FanotifyHandler& operator=(const FanotifyHandler&) =delete;

            int getFd();

        private:
            datatypes::AutoFd m_fd;
    };
}

