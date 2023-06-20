// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "unixsocket/BaseServerSocket.h"
#include "ProcessControllerServerConnectionThread.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

namespace unixsocket
{
    using ProcessControllerServerSocketBase = ImplServerSocket<ProcessControllerServerConnectionThread>;

    class ProcessControllerServerSocket : public ProcessControllerServerSocketBase
    {
    public:
        ProcessControllerServerSocket(
            const std::string& path,
            mode_t mode,
            std::shared_ptr<IProcessControlMessageCallback> processControlCallback);
        ProcessControllerServerSocket(
            const std::string& path,
            const std::string& userName,
            const std::string& groupName,
            mode_t mode,
            std::shared_ptr<IProcessControlMessageCallback> processControlCallback);
        ~ProcessControllerServerSocket() override;


    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            auto sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
            return std::make_unique<ProcessControllerServerConnectionThread>(fd, m_processControlCallback, sysCalls);
        }
        void logMaxConnectionsError() override
        {
            logError("Refusing connection: Maximum number of Process Controller threads has been reached");
        }

    private:
        std::shared_ptr<IProcessControlMessageCallback> m_processControlCallback;
    };

    using ProcessControllerServerSocketPtr = std::shared_ptr<ProcessControllerServerSocket>;
}

