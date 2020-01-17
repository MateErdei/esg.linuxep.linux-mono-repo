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
        AutoFd(const AutoFd&) = delete;
        AutoFd& operator=(const AutoFd&) = delete;
        AutoFd(AutoFd&&) noexcept;
        AutoFd& operator=(AutoFd&&) noexcept;

        ~AutoFd();

#ifdef AUTO_FD_IMPLICIT_INT
        [[nodiscard]] operator int() const { return m_fd; }
#endif /* AUTO_FD_IMPLICIT_INT */

        void reset(int fd=-1);
        void close();
        
        [[nodiscard]] int get() const { return m_fd; }
        [[nodiscard]] int fd() const { return m_fd; }

        [[nodiscard]] int release();
    private:
        int m_fd;
    };
}



