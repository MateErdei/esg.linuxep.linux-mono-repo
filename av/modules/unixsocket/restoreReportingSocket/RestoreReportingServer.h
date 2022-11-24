// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/SystemCallWrapper.h"
#include "pluginimpl/IRestoreReportProcessor.h"
#include "unixsocket/BaseServerSocket.h"
#include "unixsocket/SocketUtils.h"

namespace unixsocket
{
    class RestoreReportingServer : public BaseServerSocket
    {
    public:
        explicit RestoreReportingServer(const IRestoreReportProcessor& restoreReportProcessor);
        ~RestoreReportingServer() override;

    private:
        bool handleConnection(datatypes::AutoFd& connectionFd) override;
        void killThreads() override;

        const IRestoreReportProcessor& m_restoreReportProcessor;
    };
} // namespace unixsocket
