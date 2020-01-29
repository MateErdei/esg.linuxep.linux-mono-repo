/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerConnectionThread.h"
#include "SocketUtils.h"
#include "ScanRequest.capnp.h"

#include "datatypes/Print.h"
#include <capnp/serialize.h>

#include <stdexcept>
#include <iostream>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include <memory>

unixsocket::ScanningServerConnectionThread::ScanningServerConnectionThread(int fd)
    : m_finished(false), m_fd(fd)
{
}

bool unixsocket::ScanningServerConnectionThread::finished()
{
    return m_finished;
}

void unixsocket::ScanningServerConnectionThread::start()
{

}

void unixsocket::ScanningServerConnectionThread::notifyTerminate()
{

}
//
//static void throwOnError(int ret, const std::string& message)
//{
//    if (ret == 0)
//    {
//        return;
//    }
//    perror(message.c_str());
//    throw std::runtime_error(message);
//}

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

    errno = 0;
    int ret = recvmsg (socket, &msg, 0); // ret = bytes received
    if (ret < 0)
    {
        perror("Failed to receive fd recvmsg");
        return -1;
    }

    cmsg = CMSG_FIRSTHDR(&msg);

    memcpy (&fd, (int *) CMSG_DATA(cmsg), sizeof(int));

    return fd;
}

void unixsocket::ScanningServerConnectionThread::run()
{
    datatypes::AutoFd socket_fd(std::move(m_fd));
    PRINT("Got connection " << socket_fd.fd());
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    while (true)
    {
        // read length
        int32_t length = unixsocket::readLength(socket_fd);
        if (length < 0)
        {
            PRINT("Aborting connection: failed to read length");
            return;
        }

        PRINT("Read a length of " << length);
        if (length == 0)
        {
            PRINT("Ignoring length of zero");
            continue;
        }

        // read capn proto
        if (static_cast<uint32_t>(length) > (buffer_size * sizeof(capnp::word)))
        {
            buffer_size = 1 + length / sizeof(capnp::word);
            proto_buffer = kj::heapArray<capnp::word>(buffer_size);
        }

        ssize_t bytes_read = ::read(socket_fd, proto_buffer.begin(), length);
        if (bytes_read != length)
        {
            PRINT("Aborting connection: failed to read capn proto");
            return;
        }

        PRINT("Read capn of " << bytes_read);

        auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

        capnp::FlatArrayMessageReader messageInput(view);
        Sophos::ssplav::FileScanRequest::Reader requestReader =
                messageInput.getRoot<Sophos::ssplav::FileScanRequest>();

        std::string pathname = requestReader.getPathname();
        PRINT("Scan requested of " << pathname);

        // read fd
        int file_fd = recv_fd(socket_fd);
        if (file_fd < 0)
        {
            PRINT("Aborting connection: failed to read fd");
            return;
        }
        PRINT("Managed to get file descriptor: " << file_fd);

        datatypes::AutoFd file_fd_manager(file_fd);

        auto result = scan(file_fd_manager, pathname);
        file_fd_manager.reset();

        std::string serialised_result = result.serialise();

        if (! writeLengthAndBuffer(socket_fd, serialised_result))
        {
            PRINT("Failed to write result to unix socket");
        }
    }
}

scan_messages::ScanResponse
unixsocket::ScanningServerConnectionThread::scan(datatypes::AutoFd& fd, const std::string& file_path)
{
    char buffer[512];

    // Test reading the file
    ssize_t bytesRead = read(fd.get(), buffer, sizeof(buffer) - 1);
    buffer[bytesRead] = 0;
    std::cout << "Read " << bytesRead << " from " << file_path << '\n';

    // Test stat the file
    struct stat statbuf = {};
    ::fstat(fd.get(), &statbuf);
    std::cout << "size:" << statbuf.st_size << '\n';

    scan_messages::ScanResponse response;
    std::string contents(buffer);
    bool clean = (contents.find("EICAR") == std::string::npos);
    response.setClean(clean);
    if (!clean)
    {
        response.setThreatName("EICAR");
    }

    return response;
}
