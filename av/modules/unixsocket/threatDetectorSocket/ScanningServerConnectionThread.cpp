/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerConnectionThread.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "ScanRequest.capnp.h"
#include <capnp/serialize.h>

#include <stdexcept>
#include <iostream>
#include <utility>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

unixsocket::ScanningServerConnectionThread::ScanningServerConnectionThread(
        datatypes::AutoFd& fd,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
    : m_fd(std::move(fd))
    , m_scannerFactory(std::move(scannerFactory))
{
    if (m_fd < 0)
    {
        throw std::runtime_error("Attempting to construct ScanningServerConnectionThread with invalid socket fd");
    }

    if (m_scannerFactory.get() == nullptr)
    {
        throw std::runtime_error("Attempting to construct ScanningServerConnectionThread with null scanner factory");
    }
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

static inline bool fd_isset(int fd, fd_set* fds)
{
    assert(fd >= 0);
    return FD_ISSET(static_cast<unsigned>(fd), fds); // NOLINT
}

static inline void internal_fd_set(int fd, fd_set* fds)
{
    assert(fd >= 0);
    FD_SET(static_cast<unsigned>(fd), fds); // NOLINT
}

static int addFD(fd_set* fds, int fd, int currentMax)
{
    internal_fd_set(fd, fds);
    return std::max(fd, currentMax);
}

/**
 * Receive a single file descriptor from a unix socket
 * @param socket
 * @return
 */
static int recv_fd(int socket)
{
    int fd = -1;

    struct msghdr msg = {};
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

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);

    memcpy (&fd, (int *) CMSG_DATA(cmsg), sizeof(int));

    return fd;
}

/**
 * Parse a request.
 *
 * Placed in a function to ensure the MessageReader and view are deleted before the buffer might become invalid.
 *
 * @param proto_buffer
 * @param bytes_read
 * @return
 */
static unixsocket::ScanRequestObject parseRequest(kj::Array<capnp::word>& proto_buffer, ssize_t& bytes_read)
{
    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::FileScanRequest::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::FileScanRequest>();

    unixsocket::ScanRequestObject scanRequest;
    scanRequest.pathname = requestReader.getPathname();
    scanRequest.scanArchives = requestReader.getScanInsideArchives();
    scanRequest.scanType = requestReader.getScanType();
    scanRequest.userID = requestReader.getUserID();
    return scanRequest;
}

void unixsocket::ScanningServerConnectionThread::run()
{
    m_isRunning = true;
    announceThreadStarted();

    try
    {
        inner_run();
    }
    catch (const kj::Exception& ex)
    {
        if (ex.getType() == kj::Exception::Type::UNIMPLEMENTED)
        {
            LOGFATAL("ScanningServerConnectionThread terminated with serialisation unimplemented exception: "
                     << ex.getDescription().cStr());
        }
        else
        {
            LOGERROR(
                "ScanningServerConnectionThread terminated with serialisation exception: "
                << ex.getDescription().cStr());
        }
    }
    catch (const std::exception& ex)
    {
        LOGERROR("ScanningServerConnectionThread terminated with exception: " << ex.what());
    }
    catch (...)
    {
        LOGERROR("ScanningServerConnectionThread terminated with unknown exception");
    }
    m_isRunning = false;
}

void unixsocket::ScanningServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_fd));
    LOGDEBUG("Got connection " << socket_fd.fd());
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    int exitFD = m_notifyPipe.readFd();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = addFD(&readFDs, exitFD, max);
    max = addFD(&readFDs, socket_fd, max);
    threat_scanner::IThreatScannerPtr scanner;
    bool loggedLengthOfZero = false;

    while (true)
    {
        fd_set tempRead = readFDs;

        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);

        if (activity < 0)
        {
            LOGFATAL("Closing socket because pselect failed: " << errno);
            break;
        }
        // We don't set a timeout so something should have happened
        assert(activity != 0);

        if (fd_isset(exitFD, &tempRead))
        {
            LOGSUPPORT("Closing scanning socket thread");
            break;
        }
        else // if(fd_isset(socket_fd, &tempRead))
        {
            // If shouldn't be required - we have no timeout, and only 2 FDs in the pselect.
            // exitFD will cause break
            // therefore "else" must be fd_isset(socket_fd, &tempRead)
            assert(fd_isset(socket_fd, &tempRead));
            // read length
            int32_t length = unixsocket::readLength(socket_fd);
            if (length == -2)
            {
                LOGDEBUG("Scanning Server Connection closed: EOF");
                break;
            }
            else if (length < 0)
            {
                LOGERROR("Aborting Scanning Server Connection Thread: failed to read length");
                break;
            }
            else if (length == 0)
            {
                if (not loggedLengthOfZero)
                {
                    LOGDEBUG("Ignoring length of zero / No new messages");
                    loggedLengthOfZero = true;
                }
                continue;
            }

            // read capn proto
            if (static_cast<uint32_t>(length) > (buffer_size * sizeof(capnp::word)))
            {
                buffer_size = 1 + length / sizeof(capnp::word);
                proto_buffer = kj::heapArray<capnp::word>(buffer_size);
                loggedLengthOfZero = false;
            }

            ssize_t bytes_read = ::read(socket_fd, proto_buffer.begin(), length);
            if (bytes_read < 0)
            {
                LOGERROR("Aborting socket connection: " << errno);
                break;
            }
            else if (bytes_read != length)
            {
                LOGERROR("Aborting socket connection: failed to read entire message");
                break;
            }

            LOGDEBUG("Read capn of " << bytes_read);

            ScanRequestObject requestReader = parseRequest(proto_buffer, bytes_read);

            if (!scanner)
            {
                scanner = m_scannerFactory->createScanner(requestReader.scanArchives);
            }

            LOGDEBUG("Scan requested of " << requestReader.pathname);

            // read fd
            datatypes::AutoFd file_fd(recv_fd(socket_fd));
            if (file_fd.get() < 0)
            {
                LOGERROR("Aborting socket connection: failed to read fd");
                break;
            }
            LOGDEBUG("Managed to get file descriptor: " << file_fd.get());


            auto result = scanner->scan(file_fd, requestReader.pathname, requestReader.scanType, requestReader.userID);
            file_fd.reset();

            std::string serialised_result = result.serialise();

            try
            {
                if (!writeLengthAndBuffer(socket_fd, serialised_result))
                {
                    LOGERROR("Failed to write result to unix socket");
                }
            }
            catch (unixsocket::environmentInterruption& e)
            {
                LOGERROR("Exiting Scanning Connection Thread: " << e.what());
                break;
            }
        }
    }
}