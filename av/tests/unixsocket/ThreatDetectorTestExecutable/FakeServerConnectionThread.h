/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <datatypes/AutoFd.h>
#include <unixsocket/BaseServerConnectionThread.h>

namespace FakeDetectionServer
{
    class FakeServerConnectionThread : public unixsocket::BaseServerConnectionThread
    {
    public:
        FakeServerConnectionThread(datatypes::AutoFd socketFd, uint8_t* Data, size_t Size);

        void run() override
        {
            setIsRunning(true);
            announceThreadStarted();
            inner_run();
        }

    private:
        void inner_run();
        datatypes::AutoFd m_socketFd;
        uint8_t* m_Data;
        size_t m_Size;
    };
}


