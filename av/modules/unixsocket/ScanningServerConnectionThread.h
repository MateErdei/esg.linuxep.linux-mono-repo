/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


namespace unixsocket
{
    class ScanningServerConnectionThread
    {
    public:
        explicit ScanningServerConnectionThread(int fd);
        void run();
    };
}


