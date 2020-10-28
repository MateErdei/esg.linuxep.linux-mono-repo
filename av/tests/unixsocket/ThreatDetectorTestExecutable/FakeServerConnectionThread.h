/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <datatypes/AutoFd.h>
#include <unixsocket/BaseServerConnectionThread.h>

#include <vector>

namespace FakeDetectionServer
{
    class FakeServerConnectionThread : public unixsocket::BaseServerConnectionThread
    {
    public:
        FakeServerConnectionThread(datatypes::AutoFd socketFd, std::shared_ptr<std::vector<uint8_t>> Data);

        void run() override
        {
            setIsRunning(true);
            announceThreadStarted();
            inner_run();
            setIsRunning(false);
        }

    private:
        void inner_run();
        datatypes::AutoFd m_socketFd;
        std::shared_ptr<std::vector<uint8_t>> m_data;
    };
}


