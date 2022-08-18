//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "common/AbstractThreadPluginInterface.h"
#include "datatypes/ISystemCallWrapper.h"

#include "Common/Threads/NotifyPipe.h"

namespace sophos_on_access_process::fanotifyhandler
{
    class EventReaderThread : public common::AbstractThreadPluginInterface
    {
    public:
        EventReaderThread(int fanotifyFD, datatypes::ISystemCallWrapperSharedPtr sysCalls);
        void run();

    private:
        bool handleFanotifyEvent();

        int m_fanotifyfd;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        pid_t m_pid;
        pid_t m_ppid;
        pid_t m_sid;
    };
}
