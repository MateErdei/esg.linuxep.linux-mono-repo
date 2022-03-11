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
        ~ProcessControllerServerSocket() override;
        int monitorShutdownFd();
        int monitorReloadFd();
        bool triggeredShutdown();
        bool triggeredReload();

    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<ProcessControllerServerConnectionThread>(fd, m_shutdownPipe, m_reloadPipe);
        }
        void logMaxConnectionsError() override
        {
            logError("Refusing connection: Maximum number of Process Controller threads has been reached");
        }

    private:
        std::shared_ptr<Common::Threads::NotifyPipe> m_shutdownPipe;
        std::shared_ptr<Common::Threads::NotifyPipe> m_reloadPipe;
        bool m_signalledShutdown = false;
        bool m_signalledReload = false;
    };
}

