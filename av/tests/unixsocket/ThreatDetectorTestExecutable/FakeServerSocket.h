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
        void initializeData(uint8_t* Data, size_t Size);

        datatypes::AutoFd m_socketFd;
        uint8_t* m_Data;
        size_t m_Size;
        bool m_isRunning = false;

    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<FakeServerConnectionThread>(std::move(fd), m_Data, m_Size);
        }

    };
}