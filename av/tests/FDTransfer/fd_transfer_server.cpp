/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

//
// Created by Douglas Leeder on 19/12/2019.
//

#include <string>
#include <iostream>
#include <cassert>
//#include <cstdio>
//#include <cstdlib>
//#include <cstring>
//

//#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

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

    if (recvmsg (socket, &msg, 0) < 0)
        handle_error ("Failed to receive message");

    cmsg = CMSG_FIRSTHDR(&msg);

    memcpy (&fd, (int *) CMSG_DATA(cmsg), sizeof(int));

    return fd;
}


static bool handleConnection(int fd)
{
    std::cout << "Got connection "<< fd << std::endl;

    char receive_buffer[5];
    read(fd, receive_buffer, sizeof(receive_buffer) - 1);
    receive_buffer[4] = 0;

    std::cout << "Received:" << receive_buffer << std::endl;

    int file_fd = recv_fd(fd);

    // Test reading the file
    read(file_fd, receive_buffer, sizeof(receive_buffer) - 1);
    receive_buffer[4] = 0;
    std::cout << "File:" << receive_buffer << std::endl;

    // Test stat the file
    struct stat statbuf = {};
    ::fstat(file_fd, &statbuf);
    std::cout << "size:" << statbuf.st_size << std::endl;

    ::close(file_fd);
    ::close(fd);
    return false;
}


int main()
{
    int ret = chroot("/tmp/fd_chroot");
    if (ret != 0)
    {
        handle_error("Failed to chroot to /tmp/fd_chroot");
    }

    const std::string path = "/tmp/unix_socket";

    int server_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    assert(server_fd >= 0);

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    unlink(path.c_str());
    ret = bind(server_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    if (ret != 0)
    {
        handle_error("Failed to bind to unix socket path");
    }

    listen(server_fd, 2);

    bool terminate = false;

    while (!terminate)
    {
        int fd = ::accept(server_fd, nullptr, nullptr);
        if (fd < 0)
        {
            handle_error("Failed to accept connection");
        }
        terminate = handleConnection(fd);
    }

    close(server_fd);

    return 0;
}
