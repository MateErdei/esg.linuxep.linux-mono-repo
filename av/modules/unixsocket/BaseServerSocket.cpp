/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseServerSocket.h"

#include "Logger.h"

#include <common/FDUtils.h>

#include <stdexcept>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

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
    max = FDUtils::addFD(&readFDs, exitFD, max);
    max = FDUtils::addFD(&readFDs, m_socket_fd, max);

    int ret = ::listen(m_socket_fd, 2);
    if (ret != 0)
    {
        throw std::runtime_error("Unable to listen on socket_fd");
    }

    bool terminate = false;

    // Announce after we have started listening
    announceThreadStarted();
    LOGSUPPORT("Starting listening on socket: " << m_socketPath);

    while (!terminate)
    {
        fd_set tempRead = readFDs;

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);

        if (activity < 0)
        {
            int error = errno;
            if (error == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from pselect");
                continue;
            }

            LOGERROR("Failed socket, closing. Error: " << strerror(error)<< " (" << error << ')');
            break;
        }

        if (FDUtils::fd_isset(exitFD, &tempRead))
        {
            LOGDEBUG("Closing socket");
            break;
        }

        if(FDUtils::fd_isset(m_socket_fd, &tempRead))
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


void unixsocket::BaseServerSocket::logError([[maybe_unused]] const std::string& msg)
{
    LOGERROR(msg);
}

void unixsocket::BaseServerSocket::logDebug([[maybe_unused]] const std::string& msg)
{
    LOGDEBUG(msg);
}