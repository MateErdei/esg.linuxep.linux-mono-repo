//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "common/AbstractThreadPluginInterface.h"
#include "datatypes/ISystemCallWrapper.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/Threads/NotifyPipe.h"

namespace fs = sophos_filesystem;

namespace sophos_on_access_process::fanotifyhandler
{
    class EventReaderThread : public common::AbstractThreadPluginInterface
    {
    public:
        EventReaderThread(
            int fanotifyFD,
            datatypes::ISystemCallWrapperSharedPtr sysCalls,
            const fs::path& pluginInstall);
        void run();

    private:
        bool handleFanotifyEvent();
        std::string getFilePathFromFd(int fd);
        uid_t getUidFromPid(pid_t pid);

        int m_fanotifyfd;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        fs::path m_pluginLogDir;
        pid_t m_pid;
    };
}
