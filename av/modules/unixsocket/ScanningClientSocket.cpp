/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningClientSocket.h"
#include "SocketUtils.h"

#include <string>
#include <cstdio>
#include <cstdlib>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cassert>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

unixsocket::ScanningClientSocket::ScanningClientSocket(const std::string& socket_path)
    : m_socket_fd(-1)
{
    m_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(m_socket_fd >= 0);

    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";

    struct sockaddr_un addr = {};

    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    int ret = connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    if (ret != 0)
    {
        ::close(m_socket_fd);
        handle_error("Failed to connect to unix socket");
    }
}

unixsocket::ScanningClientSocket::~ScanningClientSocket()
{
    if (m_socket_fd >= 0)
    {
        ::close(m_socket_fd);
    }
}

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


scan_messages::ScanResponse unixsocket::ScanningClientSocket::scan(int file_fd, const std::string& filename)
{
    assert(m_socket_fd >= 0);
    scan_messages::ScanRequest request;
    request.setFd(file_fd); // file_fd owned by request now
    request.setPath(filename);
    std::string dataAsString = request.serialise();

    unixsocket::writeLength(m_socket_fd, dataAsString.size());
    ssize_t bytesWritten = write(m_socket_fd, dataAsString.c_str(), dataAsString.size());
    static_cast<void>(bytesWritten);
    if (bytesWritten < 0)
    {
        handle_error("Failed to write capn buffer to unix socket");
    }
    else if (dataAsString.size() != static_cast<unsigned>(bytesWritten))
    {
        handle_error("Failed to write complete capn buffer to unix socket");
    }

    send_fd(m_socket_fd, file_fd);

    return scan_messages::ScanResponse();
}
