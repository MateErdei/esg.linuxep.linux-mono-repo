/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"

#include <stdexcept>
#include <iostream>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>

// NOLINTNEXTLINE
#define PRINT(x) std::cerr << x << '\n'

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
    m_socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    throwIfBadFd(m_socket_fd, "Failed to create socket");


    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    unlink(path.c_str());
    int ret = bind(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    throwOnError(ret, "Failed to bind to unix socket path");
}

/**
 * Receive a single file descriptor from a unix socket
 * @param socket
 * @return
 */
static int recv_fd(int socket)
{
    int fd;

    struct msghdr msg = {};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(sizeof(int))];
    char dup[256];
    memset(buf, '\0', sizeof(buf));
    struct iovec io = { .iov_base = &dup, .iov_len = sizeof(dup) };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    int ret = recvmsg (socket, &msg, 0);
    throwOnError(ret, "Failed to receive message");

    cmsg = CMSG_FIRSTHDR(&msg);

    memcpy (&fd, (int *) CMSG_DATA(cmsg), sizeof(int));

    return fd;
}


static bool handleConnection(int fd)
{
    PRINT("Got connection "<< fd);

    char receive_buffer[5];
    int bytesRead = read(fd, receive_buffer, sizeof(receive_buffer) - 1);
    receive_buffer[bytesRead] = 0;

    std::cout << "Received:" << receive_buffer << std::endl;

    int file_fd = recv_fd(fd);

    // Test reading the file
    bytesRead = read(file_fd, receive_buffer, sizeof(receive_buffer) - 1);
    receive_buffer[bytesRead] = 0;
    std::cout << "File:" << receive_buffer << std::endl;

    // Test stat the file
    struct stat statbuf = {};
    ::fstat(file_fd, &statbuf);
    std::cout << "size:" << statbuf.st_size << std::endl;

    ::close(file_fd);
    ::close(fd);
    return false;
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

    close(m_socket_fd);

}


