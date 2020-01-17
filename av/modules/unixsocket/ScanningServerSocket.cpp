/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"
#include "Print.h"
#include "ScanningServerConnectionThread.h"

#include <stdexcept>
#include <iostream>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>

static void throwOnError(int ret, const std::string& message)
{
    if (ret == 0)
    {
        return;
    }
    perror(message.c_str());
    throw std::runtime_error(message);
}

static void throwIfBadFd(int fd, const std::string& message)
{
    if (fd >= 0)
    {
        return;
    }
    throw std::runtime_error(message);
}

unixsocket::ScanningServerSocket::ScanningServerSocket(const std::string& path)
{
    m_socket_fd.reset(socket(PF_UNIX, SOCK_STREAM, 0));
    throwIfBadFd(m_socket_fd, "Failed to create socket");


    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    unlink(path.c_str());
    int ret = bind(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    throwOnError(ret, "Failed to bind to unix socket path");
}


void unixsocket::ScanningServerSocket::run()
{
    listen(m_socket_fd, 2);

    bool terminate = false;

    while (!terminate)
    {
        int fd = ::accept(m_socket_fd, nullptr, nullptr);
        if (fd < 0)
        {
            perror("Failed to accept connection");
            terminate = true;
        }
        else
        {
            terminate = handleConnection(fd);
        }
    }

    m_socket_fd.reset();
}

bool unixsocket::ScanningServerSocket::handleConnection(int fd)
{
    ScanningServerConnectionThread thread(fd);
    thread.run();
    return false;
}


