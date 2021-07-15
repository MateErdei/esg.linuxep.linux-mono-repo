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

#include <iostream>

namespace SubscriberLib
{
    Subscriber::Subscriber(
        const std::string& socketAddress,
        Common::ZMQWrapperApi::IContextSharedPtr context,
        std::unique_ptr<SubscriberLib::IEventHandler> eventQueuePusher,
        int readLoopTimeoutMilliSeconds) :
        m_socketPath(socketAddress),
        m_readLoopTimeoutMilliSeconds(readLoopTimeoutMilliSeconds),
        m_context(context),
        m_eventHandler(std::move(eventQueuePusher))
    {
        LOGINFO("Creating subscriber listening on socket address: " << m_socketPath);
    }

    Subscriber::~Subscriber()
    {
        stop();
    }

    void Subscriber::subscribeToEvents()
    {

//        ZMQ.Poller poller = ZMQ.Poller(2)
//        poller.register(subscriber, ZMQ.Poller.POLLIN)
//        Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

        if (!m_socket)
        {
            LOGDEBUG("Getting subscriber");
            m_socket = m_context->getSubscriber();
        }
        m_socket->setTimeout(m_readLoopTimeoutMilliSeconds);
        m_socket->listen("ipc://" + m_socketPath);
        auto permissionUtils = Common::FileSystem::filePermissions();

        try
        {
            permissionUtils->chmod(m_socketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        }
        catch (const Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("Failed to set socket permissions: " << m_socketPath << " Exception: " << exception.what());
            m_running = false;
            return;
        }

        m_socket->subscribeTo("threatEvents");

        auto fs = Common::FileSystem::fileSystem();

        while (m_running)
        {
            try
            {
                if (fs->exists(m_socketPath))
                {
                    auto data = m_socket->read();
                    JournalerCommon::Event event = convertZmqDataToEvent(data);
                    m_eventHandler->handleEvent(event);
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
                // We expect EAGAIN from the socket timeout which we use so we don't block on the read.
                if (errnoFromSocketRead != EAGAIN)
                {
                    LOGERROR("Unexpected exception from socket read: " << exception.what());
                    m_running = false;
                }
            }
            catch (const std::exception& exception)
            {
                LOGERROR("Stopping subscriber. Exception thrown during subscriber read loop: " << exception.what());
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
            // If the socket dir does not exist then the whole plugin will exit. If that dir is missing then the
            // installation is unusable and this plugin shouldn't try and fix it.
            // So throw and let WD start us up again if needed.
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
                try
                {
                    fs->removeFile(m_socketPath);
                }
                catch(const std::exception& exception)
                {
                    LOGERROR("Subscriber socket removeFile failed");
                }
            }
            if (fs->exists(m_socketPath))
            {
                LOGINFO("Subscriber socket has been removed");
            }
            else
            {
                LOGERROR("Subscriber socket cleanup failed, socket still exists: " << m_socketPath);
            }

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

    JournalerCommon::Event Subscriber::convertZmqDataToEvent(Common::ZeroMQWrapper::data_t data)
    {
        if (data.size() == 2)
        {
            auto type = JournalerCommon::EventTypeMap.at(data[0]);
            JournalerCommon::Event event {type, data[1]};
            return event;
        }

        throw std::runtime_error("ZMQ Data arriving from pub sub did not contain the expected 2 minimum components");

    }
} // namespace SubscriberLib