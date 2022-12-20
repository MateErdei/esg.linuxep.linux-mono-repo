// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "scan_messages/ScanResponse.h"
#include "unixsocket/BaseServerConnectionThread.h"
#include "unixsocket/BaseServerSocket.h"

using namespace unixsocket;
class TestServerConnectionThread : public BaseServerConnectionThread
{
public:
    explicit TestServerConnectionThread(datatypes::AutoFd& fd)
        : m_socketFd{std::move(fd)}
    {}
    datatypes::AutoFd m_socketFd;
    void run() override
    {
        setIsRunning(true);
        announceThreadStarted();
        inner_run();
        setIsRunning(false);
    }
private:
    void inner_run();
    bool handleEvent(datatypes::AutoFd& socket_fd, ssize_t length);
    bool sendResponse(datatypes::AutoFd& socket_fd, const scan_messages::ScanResponse& response);
};

class FakeScanningServer : public ImplServerSocket<TestServerConnectionThread>
{
public:
    explicit FakeScanningServer(const std::string& path, mode_t mode=0700)
        : ImplServerSocket<connection_thread_t>(path, mode)
    {
        m_socketName = "TestServerSocket";
    }
protected:

    TPtr makeThread(datatypes::AutoFd& fd) override
    {
        return std::make_unique<TestServerConnectionThread>(fd);
    }

    void logMaxConnectionsError() override
    {
        logError("Refusing connection: Maximum number of test clients reached");
    }
};