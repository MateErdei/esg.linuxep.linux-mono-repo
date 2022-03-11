/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServerConnectionThread.h"
#include <unixsocket/BaseServerSocket.h>

namespace FakeDetectionServer
{
    using FakeServerSocketBase = unixsocket::ImplServerSocket<FakeServerConnectionThread>;

    class FakeServerSocket : public FakeServerSocketBase
    {
    public:
        FakeServerSocket(const std::string& path, mode_t mode);
        ~FakeServerSocket() override
        {
            // Need to do this before our members are destroyed
            requestStop();
            join();
        }

        void start();
        void initializeData(std::shared_ptr<std::vector<uint8_t>> data);
        bool m_isRunning = false;

    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<FakeServerConnectionThread>(std::move(fd), m_data);
        }

        void logMaxConnectionsError() override
        {
            logError("Refusing connection: Fake server socket has reached the maximum thread count");
        }

        datatypes::AutoFd m_socketFd;
        std::shared_ptr<std::vector<uint8_t>> m_data;
    };
}