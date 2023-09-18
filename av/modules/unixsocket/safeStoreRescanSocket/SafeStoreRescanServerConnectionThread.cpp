// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SafeStoreRescanServerConnectionThread.h"

#include "common/SaferStrerror.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/UnixSocketException.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <stdexcept>
#include <utility>

using namespace unixsocket;

SafeStoreRescanServerConnectionThread::SafeStoreRescanServerConnectionThread(
    datatypes::AutoFd& fd,
    std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager) :
    BaseServerConnectionThread("SafeStoreRescanServerConnectionThread"),
    m_fd(std::move(fd)), m_quarantineManager(std::move(quarantineManager))
{
    if (!m_fd.valid())
    {
        throw unixsocket::UnixSocketException(LOCATION, "Attempting to construct SafeStoreRescanServerConnectionThread with invalid socket fd");
    }
}

void SafeStoreRescanServerConnectionThread::run()
{
    setIsRunning(true);
    announceThreadStarted();

    try
    {
        inner_run();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Terminated " << m_threadName << " with exception: " << ex.what());
    }
    catch (...)
    {
        // Fatal since this means we have thrown something that isn't a subclass of std::exception
        LOGFATAL("Terminated " << m_threadName << " with unknown exception");
    }
    setIsRunning(false);
}

void SafeStoreRescanServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_fd));
    LOGDEBUG(m_threadName << " got connection " << socket_fd.fd());
    const uint32_t bufferSize = 1;

    int exitFD = m_notifyPipe.readFd();

    bool loggedLengthOfZero = false;

    struct pollfd fds[] {
        { .fd = exitFD, .events = POLLIN, .revents = 0 },
        { .fd = socket_fd, .events = POLLIN, .revents = 0 }
    };
    auto sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();

    while (true)
    {

        int activity = ::ppoll(fds, std::size(fds), nullptr, nullptr);

        if (activity < 0)
        {
            // error in ppoll
            int error = errno;
            if (error == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from ppoll");
                continue;
            }

            LOGERROR("Closing " << m_threadName << ",Error: "
                     << common::safer_strerror(error) << " (" << error << ')');
            break;
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
            LOGDEBUG("Closing " << m_threadName);
            break;
        }
        else
        {
            // If shouldn't be required - we have no timeout, and only 2 FDs in the ppoll.
            // exitFD will cause break
            // therefore "else" must be fd_isset(socket_fd, &tempRead)
            assert((fds[1].revents & POLLIN) != 0);

            // read length
            auto length = unixsocket::readLength(socket_fd);
            if (length == unixsocket::SU_EOF)
            {
                LOGDEBUG(m_threadName << " closed: EOF");
                break;
            }
            else if (length == unixsocket::SU_ERROR)
            {
                LOGERROR("Aborting " << m_threadName << ": failed to read length");
                break;
            }
            else if (length == unixsocket::SU_ZERO)
            {
                if (not loggedLengthOfZero)
                {
                    LOGDEBUG(m_threadName << " ignoring length of zero / No new messages");
                    loggedLengthOfZero = true;
                }
                continue;
            }

            char buffer[bufferSize];
            auto charsRead = unixsocket::readFully(sysCalls,
                                                       socket_fd.get(),
                                                       buffer,
                                                       bufferSize,
                                                       readTimeout_);
            if (charsRead < 0)
            {
                LOGERROR("Aborting " << m_threadName << errno);
                break;
            }
            else if (charsRead == bufferSize && buffer[0] == '1')
            {
                m_quarantineManager->rescanDatabase();
            }
        }
    }
}
