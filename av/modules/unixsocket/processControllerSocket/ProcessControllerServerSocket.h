// Copyright 2020-2022, Sophos Limited.  All rights reserved.

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
            const mode_t mode,
            std::shared_ptr<IProcessControlMessageCallback> processControlCallback);
        ~ProcessControllerServerSocket() override;


    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<ProcessControllerServerConnectionThread>(fd, m_processControlCallback);
        }
        void logMaxConnectionsError() override
        {
            logError("Refusing connection: Maximum number of Process Controller threads has been reached");
        }

    private:
        std::shared_ptr<IProcessControlMessageCallback> m_processControlCallback;
    };
}

