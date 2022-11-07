// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreRescanServerConnectionThread.h"

#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include "common/FDUtils.h"

#include <cassert>
#include <stdexcept>
#include <utility>

#include <sys/socket.h>
#include <unistd.h>

using namespace unixsocket;

SafeStoreRescanServerConnectionThread::SafeStoreRescanServerConnectionThread(
    datatypes::AutoFd& fd,
    std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager) :
    m_fd(std::move(fd)), m_quarantineManager(std::move(quarantineManager))
{
    if (m_fd < 0)
    {
        throw std::runtime_error("Attempting to construct SafeStoreRescanServerConnectionThread with invalid socket fd");
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
        LOGERROR("Terminated SafeStoreRescanServerConnectionThread with exception: " << ex.what());
    }
    catch (...)
    {
        // Fatal since this means we have thrown something that isn't a subclass of std::exception
        LOGFATAL("Terminated SafeStoreRescanServerConnectionThread with unknown exception");
    }
    setIsRunning(false);
}

void SafeStoreRescanServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_fd));
    LOGDEBUG("SafeStore Rescan Server thread got connection " << socket_fd.fd());
    uint32_t bufferSize = 1;

    int exitFD = m_notifyPipe.readFd();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = FDUtils::addFD(&readFDs, exitFD, max);
    max = FDUtils::addFD(&readFDs, socket_fd, max);
    bool loggedLengthOfZero = false;

    while (true)
    {
        fd_set tempRead = readFDs;

        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);

        if (activity < 0)
        {
            LOGERROR("Closing SafeStore Rescan connection thread, error: " << errno);
            break;
        }

        if (FDUtils::fd_isset(exitFD, &tempRead))
        {
            LOGDEBUG("Closing SafeStore Rescan connection thread");
            break;
        }
        else
        // if (FDUtils::fd_isset(socket_fd, &tempRead))
        {
            // If shouldn't be required - we have no timeout, and only 2 FDs in the pselect.
            // exitFD will cause break
            // therefore "else" must be fd_isset(socket_fd, &tempRead)
            assert(FDUtils::fd_isset(socket_fd, &tempRead));

            // read length
            int32_t length = unixsocket::readLength(socket_fd);
            if (length == -2)
            {
                LOGDEBUG("SafeStore Rescan connection thread closed: EOF");
                break;
            }
            else if (length < 0)
            {
                LOGERROR("Aborting SafeStore Rescan connection thread: failed to read length");
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

            char buffer[bufferSize];
            auto charsRead = ::read(socket_fd.get(), buffer, bufferSize);
            if (charsRead < 0)
            {
                LOGERROR("Aborting SafeStore Rescan connection thread: " << errno);
                break;
            }
            else if (charsRead == bufferSize && buffer[0] == '1')
            {
                m_quarantineManager->rescanDatabase();
            }
        }
    }
}
