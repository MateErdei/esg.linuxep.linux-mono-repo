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
        int monitorEnableFd();
        int monitorDisableFd();

        bool triggeredShutdown();
        bool triggeredReload();
        bool triggeredEnable();
        bool triggeredDisable();

    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<ProcessControllerServerConnectionThread>(fd,
                                                                             m_shutdownPipe,
                                                                             m_reloadPipe,
                                                                             m_enablePipe,
                                                                             m_disablePipe);
        }
        void logMaxConnectionsError() override
        {
            logError("Refusing connection: Maximum number of Process Controller threads has been reached");
        }

    private:
        std::shared_ptr<Common::Threads::NotifyPipe> m_shutdownPipe;
        std::shared_ptr<Common::Threads::NotifyPipe> m_reloadPipe;
        std::shared_ptr<Common::Threads::NotifyPipe> m_enablePipe;
        std::shared_ptr<Common::Threads::NotifyPipe> m_disablePipe;

        bool m_signalledShutdown = false;
        bool m_signalledReload = false;
        bool m_signalledEnable = false;
        bool m_signalledDisable = false;
    };
}

