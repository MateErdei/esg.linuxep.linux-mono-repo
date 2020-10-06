/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace unixsocket
{
    int readLength(int socketfd);
    void writeLength(int socket_fd, unsigned length);
    bool writeLengthAndBuffer(int socketfd, const std::string& buffer);
    int recv_fd(int socket);
    int send_fd(int socket, int fd);

    struct environmentInterruption : public std::exception
    {
        [[nodiscard]] const char * what () const noexcept override
        {
            return "Environment interruption";
        }
    };
}
