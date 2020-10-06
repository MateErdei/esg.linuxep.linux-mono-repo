/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketUtils.h"
#include "SocketUtilsImpl.h"

#include "Logger.h"

#include <cstdint>
#include <cstring>
#include <memory>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

void unixsocket::writeLength(int socket_fd, unsigned length)
{
    if (length == 0)
    {
        throw std::runtime_error("Attempting to write length of zero");
    }

    auto bytes = splitInto7Bits(length);
    auto buffer = addTopBitAndPutInBuffer(bytes);
    ssize_t bytes_written;
    bytes_written = ::send(socket_fd, buffer.get(), bytes.size(), MSG_NOSIGNAL);
    if (bytes_written != static_cast<ssize_t>(bytes.size()))
    {
        throw environmentInterruption();
    }
}

bool unixsocket::writeLengthAndBuffer(int socket_fd, const std::string& buffer)
{
    writeLength(socket_fd, buffer.size());

    ssize_t bytes_written = ::send(socket_fd, buffer.c_str(), buffer.size(), MSG_NOSIGNAL);
    if (static_cast<unsigned>(bytes_written) != buffer.size())
    {
        LOGWARN("Failed to write buffer to unix socket");
        return false;
    }
    return true;
}

int unixsocket::readLength(int socket_fd)
{
    int32_t total = 0;
    uint8_t byte; // For some reason clang-tidy thinks this is signed
    const uint8_t TOP_BIT = 0x80;
    while (true)
    {
        ssize_t count = read(socket_fd, &byte, 1);
        if (count == 1)
        {
            if ((byte & TOP_BIT) == 0)
            {
                total = total * 128 + byte;
                return total;
            }
            total = total * 128 + (byte ^ TOP_BIT);
            if (total > 128 * 1024)
            {
                return -1;
            }
        }
        else if (count == -1)
        {
            LOGERROR("Reading socket returned error: " << std::strerror(errno));
            return -1;
        }
        else if (count == 0)
        {
            // can't call perror since 0 return doesn't set errno
            // should only return count == 0 on EOF, so we don't need to continue the connection
            return -2;
        }
    }
}

/**
 * Receive a single file descriptor from a unix socket
 * @param socket
 * @return
 */
int unixsocket::recv_fd(int socket)
{
    int fd = -1;

    struct msghdr msg = {};
    char buf[CMSG_SPACE(sizeof(int))] {};
    char dup[256];
    struct iovec io = { .iov_base = &dup, .iov_len = sizeof(dup) };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    errno = 0;
    ssize_t ret = recvmsg (socket, &msg, 0); // ret = bytes received
    if (ret < 0)
    {
        perror("Failed to receive fd: recvmsg failed");
        return -1;
    }

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);

    if ( cmsg == nullptr )
    {
        LOGDEBUG("Failed to receive fd: CMSG_FIRSTHDR failed");
        return -1;
    }

    if ( cmsg->cmsg_level != SOL_SOCKET
         || cmsg->cmsg_type != SCM_RIGHTS
         || cmsg->cmsg_len != CMSG_LEN(sizeof(int) )
        )
    {
        LOGDEBUG("Failed to receive fd: cmsg is not a file descriptor");
        return -1;
    }

    memcpy (&fd, (int *) CMSG_DATA(cmsg), sizeof(int));

    return fd;
}

int unixsocket::send_fd(int socket, int fd)  // send fd by socket
{
    struct msghdr msg = {};
    char buf[CMSG_SPACE(sizeof(fd))] {};
    char dup[256] {};
    struct iovec io = { .iov_base = &dup, .iov_len = sizeof(dup) };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

    memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));

    return sendmsg(socket, &msg, MSG_NOSIGNAL);
}