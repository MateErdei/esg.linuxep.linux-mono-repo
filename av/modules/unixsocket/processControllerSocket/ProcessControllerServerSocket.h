// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/SystemCallWrapper.h"
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
            auto sysCalls = std::make_shared<datatypes::SystemCallWrapper>();
            return std::make_unique<ProcessControllerServerConnectionThread>(fd, m_processControlCallback, sysCalls);
        }
        void logMaxConnectionsError() override
        {
            logError("Refusing connection: Maximum number of Process Controller threads has been reached");
        }

    private:
        std::shared_ptr<IProcessControlMessageCallback> m_processControlCallback;
    };
}

