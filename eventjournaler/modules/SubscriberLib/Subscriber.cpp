// Copyright 2021-2023 Sophos Limited. All rights reserved.

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
#include <utility>
#include <modules/Heartbeat/Heartbeat.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/pluginimpl/TelemetryConsts.h>

namespace SubscriberLib
{
    Subscriber::Subscriber(
        std::string socketAddress,
        Common::ZMQWrapperApi::IContextSharedPtr context,
        std::unique_ptr<SubscriberLib::IEventHandler> eventQueuePusher,
        std::shared_ptr<Heartbeat::HeartbeatPinger> heartbeatPinger,
        int readLoopTimeoutMilliSeconds) :
        m_socketPath(std::move(socketAddress)),
        m_readLoopTimeoutMilliSeconds(readLoopTimeoutMilliSeconds),
        m_context(std::move(context)),
        m_eventHandler(std::move(eventQueuePusher)),
        m_heartbeatPinger(std::move(heartbeatPinger))
    {
        LOGINFO("Creating subscriber listening on socket address: " << m_socketPath);
    }

    Subscriber::~Subscriber()
    {
        stop();
    }

    void Subscriber::subscribeToEvents()
    {
        setIsRunning(true);
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
            setIsRunning(false);
            return;
        }

        m_socket->subscribeTo("threatEvents");

        auto fs = Common::FileSystem::fileSystem();

        while (shouldBeRunning())
        {
            try
            {
                m_heartbeatPinger->ping();
                if (fs->exists(m_socketPath))
                {
                    auto data = m_socket->read();
                    std::optional<JournalerCommon::Event> event = convertZmqDataToEvent(data);
                    if (event)
                    {
                        m_eventHandler->handleEvent(event.value());
                    }
                    else
                    {
                        LOGERROR("ZMQ data arriving from pub sub could not be converted to Event");
                    }
                }
                else
                {
                    LOGERROR("The subscriber socket has been unexpectedly removed.");
                    setIsRunning(false);
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
                    setIsRunning(false);
                    break;
                }
            }
            catch (const std::exception& exception)
            {
                LOGERROR("Stopping subscriber. Exception thrown during subscriber read loop: " << exception.what());
                setIsRunning(false);
                break;
            }
        }
        fs->removeFile(m_socketPath);
        setIsRunning(false);
    }

    void Subscriber::start()
    {
        if (m_runnerThread)
        {
            LOGWARN("Subscriber thread already running, skipping start call.");
            return;
        }
        LOGINFO("Starting Subscriber");
        setShouldBeRunning(true);
        auto fs = Common::FileSystem::fileSystem();
        std::string socketDir = Common::FileSystem::dirName(m_socketPath);
        if (fs->isDirectory(socketDir))
        {
            if (fs->exists(m_socketPath))
            {
                try
                {
                    fs->removeFile(m_socketPath);
                }
                catch(const std::exception& exception)
                {
                    std::string msg =
                            "Subscriber start failed to remove existing socket: " + m_socketPath +
                            " Error: " + exception.what();
                    LOGERROR(msg);
                    throw std::runtime_error(msg);
                }
            }
        }
        else
        {

            std::string msg = "The events pub/sub socket directory does not exist: " + socketDir;
            LOGERROR(msg);
            // If the socket dir does not exist then the whole plugin will exit. If that dir is missing then the
            // installation is unusable and this plugin shouldn't try and fix it.
            // So throw and let WD start us up again if needed.
            throw std::runtime_error(msg);
        }
        m_runnerThread = std::make_unique<std::thread>(std::thread([this] { subscribeToEvents(); }));
        LOGINFO("Subscriber started");
    }

    void Subscriber::stop() noexcept
    {
        LOGINFO("Stopping Subscriber");
        setShouldBeRunning(false);
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
                catch (const std::exception& exception)
                {
                    LOGERROR("Subscriber socket removeFile failed");
                }
            }
            if (!fs->exists(m_socketPath))
            {
                LOGINFO("Subscriber socket has been removed");
            }
            else
            {
                LOGERROR("Subscriber socket cleanup failed, socket still exists: " << m_socketPath);
            }

        }
        LOGINFO("Subscriber stopped");
        setIsRunning(false);
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

    bool Subscriber::getRunningStatus()
    {
        auto lock = m_isRunning.lock();
        return *lock;
    }

    bool Subscriber::shouldBeRunning()
    {
        auto lock = m_shouldBeRunning.lock();
        return *lock;
    }

    void Subscriber::setIsRunning(bool value)
    {
        auto lock = m_isRunning.lock();
        *lock = value;
    }

    void Subscriber::setShouldBeRunning(bool value)
    {
        auto lock = m_shouldBeRunning.lock();
        *lock = value;
    }

    std::optional<JournalerCommon::Event> Subscriber::convertZmqDataToEvent(Common::ZeroMQWrapper::data_t data)
    {
        if (data.size() == 2)
        {
            auto type = JournalerCommon::PubSubSubjectToEventTypeMap.at(data[0]);
            JournalerCommon::Event event {type, data[1]};
            return event;
        }
        return {};
    }

} // namespace SubscriberLib