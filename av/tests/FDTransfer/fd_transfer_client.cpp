/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

//
// Created by Douglas Leeder on 19/12/2019.
//

#include <string>
#include <cstdio>
#include <cstdlib>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

static
void send_fd(int socket, int fd)  // send fd by socket
{
    struct msghdr msg = {};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(sizeof(fd))];
    char dup[256];
    memset(buf, '\0', sizeof(buf));
    struct iovec io = { .iov_base = &dup, .iov_len = sizeof(dup) };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

    memcpy (CMSG_DATA(cmsg), &fd, sizeof (fd));

    if (sendmsg (socket, &msg, 0) < 0)
    {
        handle_error ("Failed to send message");
    }
}


int main(int argc, char* argv[])
{
    std::string filename = "/etc/fstab";
    if (argc > 1)
    {
        filename = argv[1];
    }

    int file_fd = open(filename.c_str(), O_RDONLY);
    assert(file_fd >= 0);

    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(socket_fd >= 0);

    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    int ret = connect(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    if (ret != 0)
    {
        handle_error("Failed to connect to unix socket");
    }

    write(socket_fd, "ABCD", 4);

    send_fd(socket_fd, file_fd);

    close(file_fd);
    close(socket_fd);

    return 0;
}
