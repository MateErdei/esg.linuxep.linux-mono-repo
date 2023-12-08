// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "scan_messages/ScanResponse.h"
#include "unixsocket/BaseServerConnectionThread.h"
#include "unixsocket/BaseServerSocket.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

using namespace unixsocket;
class TestServerConnectionThread : public BaseServerConnectionThread
{
public:
    explicit TestServerConnectionThread(datatypes::AutoFd& fd)
        :
        BaseServerConnectionThread("TestServerConnectionThread")
        , m_socketFd{std::move(fd)}
    {}
    datatypes::AutoFd m_socketFd;
    void run() override
    {
        setIsRunning(true);
        announceThreadStarted();
        inner_run();
        setIsRunning(false);
    }
    std::string m_nextResponse;
    bool m_sendGiantResponse;

private:
    void inner_run();
    bool handleEvent(datatypes::AutoFd& socket_fd, ssize_t length);
    bool sendResponse(datatypes::AutoFd& socket_fd, const scan_messages::ScanResponse& response);
    bool sendResponse(datatypes::AutoFd& socket_fd, const std::string& response);
    bool sendResponse(int socket_fd, size_t length, const std::string& buffer);
    Common::SystemCallWrapper::SystemCallWrapper systemCallWrapper_;
};

class FakeScanningServer : public ImplServerSocket<TestServerConnectionThread>
{
public:
    explicit FakeScanningServer(const std::string& path, mode_t mode=0700)
        :
        ImplServerSocket<connection_thread_t>(path, "TestServerSocket", mode)
    {
    }
    std::string m_nextResponse;
    TestServerConnectionThread* m_latestThread = nullptr; // Borrowed pointer to latested thread
    bool m_sendGiantResponse = false;

protected:
    TPtr makeThread(datatypes::AutoFd& fd) override
    {
        auto t = std::make_unique<TestServerConnectionThread>(fd);
        m_latestThread = t.get();
        t->m_nextResponse = m_nextResponse;
        t->m_sendGiantResponse = m_sendGiantResponse;
        return t;
    }

    void logMaxConnectionsError() override
    {
        logError("Refusing connection: Maximum number of test clients reached");
    }
};