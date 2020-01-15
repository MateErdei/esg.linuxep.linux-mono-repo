/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


namespace scan_messages
{
    class AutoFd
    {
    public:
        explicit AutoFd(int fd=-1);
        ~AutoFd();
        void reset(int fd=-1);
        int get() { return m_fd; }
    private:
        int m_fd;
    };
}



