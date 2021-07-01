/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Subscriber.h"

#include "Logger.h"

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/ProjectNames.h>
#include <Common/UtilityImpl/WaitForUtils.h>
#include <Common/ZeroMQWrapper/IIPCException.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <sys/stat.h>

#include <cstring>
#include <iostream>
#include <unistd.h>

namespace SubscriberLib
{
    Subscriber::Subscriber(
        const std::string& socketAddress,
        Common::ZMQWrapperApi::IContextSharedPtr context,
        int readLoopTimeoutMilliSeconds) :
        m_socketPath(socketAddress), m_readLoopTimeoutMilliSeconds(readLoopTimeoutMilliSeconds), m_context(context)
    {
        LOGINFO("Creating subscriber listening on socket address: " << m_socketPath);
    }

    Subscriber::~Subscriber()
    {
        LOGINFO("Stop not explicitly called, calling in destructor.");
        stop();
    }

    void Subscriber::subscribeToEvents()
    {
        if (!m_socket)
        {
            LOGDEBUG("Getting subscriber");
            m_socket = m_context->getSubscriber();
        }
        m_socket->setTimeout(m_readLoopTimeoutMilliSeconds);
        m_socket->listen("ipc://" + m_socketPath);
        m_socket->subscribeTo("threatEvents");

        auto fs = Common::FileSystem::fileSystem();

        while (m_running)
        {
            try
            {
                if (fs->exists(m_socketPath))
                {
                    auto data = m_socket->read();
                    LOGINFO("Received event");
                    int index = 0;
                    for (const auto& messagePart : data)
                    {
                        LOGINFO(index++ << ": " << messagePart);
                    }
                }
                else
                {
                    LOGERROR("The subscriber socket has been unexpectedly removed.");
                    m_running = false;
                    return;
                }
            }
            catch (const Common::ZeroMQWrapper::IIPCException& exception)
            {
                int errnoFromSocketRead = errno;
                LOGINFO(std::to_string(errnoFromSocketRead));
                // We expect EAGAIN from the socket timeout which we use so we don't block on the read.
                if (errnoFromSocketRead != EAGAIN)
                {
                    LOGERROR("Unexpected exception from socket read: " << exception.what());
                    m_running = false;
                }
            }
            catch (const std::exception& exception)
            {
                m_running = false;
            }
        }
        fs->removeFile(m_socketPath);
    }

    void Subscriber::start()
    {
        LOGINFO("Starting Subscriber");

        auto fs = Common::FileSystem::fileSystem();
        std::string socketDir = Common::FileSystem::dirName(m_socketPath);
        if (fs->isDirectory(socketDir))
        {
            if (fs->exists(m_socketPath))
            {
                fs->removeFileOrDirectory(m_socketPath);
            }
        }
        else
        {
            m_running = false;
            std::string msg = "The events pub/sub socket directory does not exist: " + socketDir;
            LOGERROR(msg);
            throw std::runtime_error(msg);
        }
        m_running = true;
        m_runnerThread = std::make_unique<std::thread>(std::thread([this] { subscribeToEvents(); }));
        LOGINFO("Subscriber started");
    }

    void Subscriber::stop()
    {
        LOGINFO("Stopping Subscriber");
        m_running = false;
        if (m_runnerThread && m_runnerThread->joinable())
        {
            m_runnerThread->join();
            m_runnerThread.reset();
        }
        LOGDEBUG("Thread has been stopped");
        auto fs = Common::FileSystem::fileSystem();
        if (fs->exists(m_socketPath))
        {
            LOGINFO("Waiting for subscriber socket to be removed");
            bool socketRemoved =
                Common::UtilityImpl::waitFor(1, 0.1, [this, fs]() { return (!fs->exists(m_socketPath)); });

            if (!socketRemoved)
            {
                LOGERROR("Subscriber socket was not removed after Subscriber finished, deleting: " << m_socketPath);
                fs->removeFile(m_socketPath);
            }
            LOGINFO("Subscriber socket has been removed");
        }
        LOGINFO("Subscriber stopped");
    }

    void Subscriber::restart()
    {
        LOGINFO("Subscriber restart called");
        auto fs = Common::FileSystem::fileSystem();
        stop();
        start();
        bool socketExists = Common::UtilityImpl::waitFor(5, 0.1, [this, fs]() { return fs->exists(m_socketPath); });
        if (!socketExists)
        {
            LOGERROR("Socket was not created after starting subscriber thread");
        }
    }

    bool Subscriber::getRunningStatus() { return m_running; }
} // namespace SubscriberLib