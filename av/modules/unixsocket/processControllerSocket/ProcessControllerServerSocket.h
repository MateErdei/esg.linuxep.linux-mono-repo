/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "unixsocket/BaseServerSocket.h"
#include "ProcessControllerServerConnectionThread.h"

namespace unixsocket
{
    using ProcessControllerServerSocketBase = ImplServerSocket<ProcessControllerServerConnectionThread>;

    class ProcessControllerServerSocket : public ProcessControllerServerSocketBase
    {
    public:
        ProcessControllerServerSocket(
            const std::string& path,
            const mode_t mode);
        int monitorFd();
        bool triggered();

    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<ProcessControllerServerConnectionThread>(fd, m_shutdownPipe);
        }

    private:
        std::shared_ptr<Common::Threads::NotifyPipe> m_shutdownPipe;
        bool m_signalled = false;
    };
}

