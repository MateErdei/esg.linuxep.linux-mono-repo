/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningServerConnectionThread.h"
#include "Logger.h"

#include <stdexcept>

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

unixsocket::BaseServerSocket::BaseServerSocket(const sophos_filesystem::path& path, const mode_t mode)
    : m_socketPath(path)
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

    ::chmod(path.c_str(), mode);
}

unixsocket::BaseServerSocket::~BaseServerSocket()
{
    ::unlink(m_socketPath.c_str());
}

void unixsocket::BaseServerSocket::run()
{
    int exitFD = m_notifyPipe.readFd();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = addFD(&readFDs, exitFD, max);
    max = addFD(&readFDs, m_socket_fd, max);

    listen(m_socket_fd, 2);

    bool terminate = false;

    // Announce after we have started listening
    announceThreadStarted();
    // TODO: Should this be LOGSUPPORT?
    LOGINFO("Starting    listening on socket");

    while (!terminate)
    {
        fd_set tempRead = readFDs;

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);

        if (activity < 0)
        {
            // handle error
            LOGERROR("Closing    socket because it failed: " << errno);
            break;
        }

        if (fd_isset(exitFD, &tempRead))
        {
            // TODO: Should this be LOGSUPPORT?
            LOGINFO("Closing    socket");
            break;
        }

        if(fd_isset(m_socket_fd, &tempRead))
        {
            datatypes::AutoFd client_socket(
                ::accept(m_socket_fd, nullptr, nullptr)
            );

            if (client_socket.get() < 0)
            {
                perror("Failed to accept connection");
                terminate = true;
            }
            else
            {
                terminate = handleConnection(client_socket);
            }
        }
    }

    ::unlink(m_socketPath.c_str());
    m_socket_fd.reset();
    killThreads();
}
