// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

namespace datatypes
{
    class AutoFd
    {
    public:
        explicit AutoFd(int fd=invalid_fd) noexcept;
        AutoFd(const AutoFd&) = delete;
        AutoFd& operator=(const AutoFd&) = delete;
        AutoFd(AutoFd&&) noexcept;
        AutoFd& operator=(AutoFd&&) noexcept;
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

        // operator== deleted, as no two AutoFd objects should ever reference the same fd
        bool operator==(const AutoFd& other) = delete;

        void reset(int fd=invalid_fd);
        void close();

        [[nodiscard]] int get() const { return m_fd; }
        [[nodiscard]] int fd() const { return m_fd; }

        [[nodiscard]] int release();
    private:
        int m_fd;
        static constexpr int invalid_fd = -1;
    };
}



