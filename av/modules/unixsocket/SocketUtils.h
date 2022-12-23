/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <string>

namespace unixsocket
{
    ssize_t readLength(int socket_fd);
    using buffer_ptr_t = std::unique_ptr<uint8_t[]>;
    buffer_ptr_t getLength(size_t length, size_t& bufferSize);

    void writeLength(int socket_fd, size_t length);
    bool writeLengthAndBuffer(int socket_fd, const std::string& buffer);
    int recv_fd(int socket);
    ssize_t send_fd(int socket, int fd);

    bool writeLengthAndBufferAndFd(int socket_fd, const std::string& buffer, int fd);

    struct environmentInterruption : public std::exception
    {
        [[nodiscard]] const char * what () const noexcept override
        {
            return "Environment interruption";
        }
    };

    const int SU_EOF = -2;
    const int SU_ERROR = -1;
    const int SU_ZERO = 0;
}
