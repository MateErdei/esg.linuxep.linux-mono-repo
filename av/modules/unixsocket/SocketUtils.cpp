// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "SocketUtils.h"

#include "Logger.h"
#include "SocketUtilsImpl.h"

#include "common/SaferStrerror.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

void unixsocket::writeLength(int socket_fd, size_t length)
{
    size_t bufferSize;
    auto buffer = getLength(length, bufferSize);
    ssize_t bytes_written
        = ::send(socket_fd, buffer.get(), bufferSize, MSG_NOSIGNAL);
    if (bytes_written != static_cast<ssize_t>(bufferSize))
    {
        throw EnvironmentInterruption(__FUNCTION__);
    }
}

bool unixsocket::writeLengthAndBuffer(
    Common::SystemCallWrapper::ISystemCallWrapper& systemCallWrapper,
    int socket_fd,
    const std::string& buffer)
{
    struct iovec iov[2];
    size_t lengthBufferSize;
    auto lengthBuffer = getLength(buffer.size(), lengthBufferSize);
    iov[0].iov_base = lengthBuffer.get();
    iov[0].iov_len = lengthBufferSize;
    iov[1].iov_base = const_cast<char*>(buffer.data());
    iov[1].iov_len = buffer.size();

    struct msghdr message{};
    message.msg_iov = iov;
    message.msg_iovlen = 2;

    ssize_t bytes_written = systemCallWrapper.sendmsg(socket_fd, &message, MSG_NOSIGNAL);
    if (static_cast<unsigned>(bytes_written) != buffer.size() + lengthBufferSize)
    {
        throw EnvironmentInterruption(__FUNCTION__);
    }
    return true;
}

ssize_t unixsocket::readLength(int socket_fd, ssize_t maxSize)
{
    int32_t total = 0;
    uint8_t byte = 0; // For some reason clang-tidy thinks this is signed
    const uint8_t TOP_BIT = 0x80;
    while (true)
    {
        ssize_t count = ::read(socket_fd, &byte, 1);
        if (count == 1)
        {
            if ((byte & TOP_BIT) == 0)
            {
                total = total * 128 + byte;
                return total;
            }
            total = total * 128 + (byte ^ TOP_BIT);
            if (total > maxSize)
            {
                return -1;
            }
        }
        else if (count == -1)
        {
            LOGDEBUG("Reading socket returned error: " << common::safer_strerror(errno));
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
 */
int unixsocket::recv_fd(Common::SystemCallWrapper::ISystemCallWrapper& systemCallWrapper, int socket)
{
    int fd = -1;

    struct msghdr msg = {};
    char buf[CMSG_SPACE(sizeof(int))] {};
    int dup = 0;
    struct iovec io = { .iov_base = &dup, .iov_len = sizeof(dup) };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    errno = 0;
    ssize_t ret = systemCallWrapper.recvmsg(socket, &msg, 0); // ret = bytes received
    if (ret < 0)
    {
        perror("Failed to receive fd: recvmsg failed");
        return -1;
    }

    if (msg.msg_flags & MSG_TRUNC) // NOLINT
    {
        LOGERROR("Message was truncated when receiving fd");
    }

    if (msg.msg_flags & MSG_CTRUNC) // NOLINT
    {
        LOGERROR("Control data was truncated when receiving fd");
    }

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);

    if ( cmsg == nullptr )
    {
        LOGDEBUG("Failed to receive fd: CMSG_FIRSTHDR failed");
        return -1;
    }

    if ( cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
    {
        LOGDEBUG("Failed to receive fd: cmsg is not a file descriptor");
        return -1;
    }

    if(cmsg->cmsg_len != CMSG_LEN(sizeof(int) ))
    {
        LOGERROR("Failed to receive fd: got wrong number of fds");
        // close any fds we did receive, else we leak them
        unsigned char *start = CMSG_DATA(cmsg);
        unsigned char *end =  reinterpret_cast<unsigned char *>(cmsg) + cmsg->cmsg_len;
        std::vector<int> fds((end - start) / sizeof(int));
        memcpy(fds.data(), start, end - start);
        for (int temp_fd: fds)
        {
            LOGDEBUG("Closing fd: " << temp_fd);
            close(temp_fd);
        }
        return -1;
    }

    memcpy (&fd, (int *) CMSG_DATA(cmsg), sizeof(int));

    return fd;
}

ssize_t unixsocket::send_fd(int socket, int fd)  // send fd by socket
{
    char buf[CMSG_SPACE(sizeof(fd))] {};
    int dup = 0;
    struct iovec io = { .iov_base = &dup, .iov_len = sizeof(dup) };

    struct msghdr msg = {};
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

bool unixsocket::writeLengthAndBufferAndFd(
    Common::SystemCallWrapper::ISystemCallWrapper& systemCallWrapper,
    int socket_fd,
    const std::string& buffer,
    int fd)
{
    std::ignore = writeLengthAndBuffer(systemCallWrapper, socket_fd, buffer); // throws or returns true
    return (send_fd(socket_fd, fd) > 0);
}

ssize_t unixsocket::readFully(
    const Common::SystemCallWrapper::ISystemCallWrapperSharedPtr& syscalls,
    int socket_fd, char* buf, ssize_t bytes, std::chrono::milliseconds timeout)
{
    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    ssize_t totalRead = 0;
    while (totalRead < bytes)
    {
        auto remaining = bytes - totalRead;
        auto amount = syscalls->read(socket_fd, buf + totalRead, remaining);
        if (amount < 0)
        {
            return amount;
        }
        totalRead += amount;
        auto now = clock::now();
        if (now - start > timeout)
        {
            return totalRead;
        }
    }
    return totalRead;
}