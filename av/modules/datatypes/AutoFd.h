// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

namespace datatypes
{
    class AutoFd
    {
    public:
        explicit AutoFd(int fd=-1) noexcept;
        AutoFd(const AutoFd&) = delete;
        AutoFd& operator=(const AutoFd&) = delete;
        AutoFd(AutoFd&&) noexcept;
        AutoFd& operator=(AutoFd&&) noexcept;
        bool operator==(const AutoFd& other) const;

        ~AutoFd();

#ifdef AUTO_FD_IMPLICIT_INT
        [[nodiscard]] operator int() const { return m_fd; }
        AutoFd& operator=(int fd)
        {
            reset(fd);
            return *this;
        }
#endif /* AUTO_FD_IMPLICIT_INT */

        [[nodiscard]] bool valid() const
        {
            return m_fd >= 0;
        }

#ifdef AUTO_FD_IMPLICIT_BOOL
        [[nodiscard]] operator bool() const { return valid(); }
#endif /* AUTO_FD_IMPLICIT_BOOL */

        void reset(int fd=-1);
        void close();

        [[nodiscard]] int get() const { return m_fd; }
        [[nodiscard]] int fd() const { return m_fd; }

        [[nodiscard]] int release();
    private:
        int m_fd;
    };
}



