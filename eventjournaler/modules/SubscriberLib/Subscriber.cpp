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

#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/ZeroMQWrapper/IIPCException.h>
#include <sys/stat.h>

#include <cstring>
#include <iostream>
#include <unistd.h>

namespace SubscriberLib
{
    Subscriber::Subscriber(const std::string& socketAddress, Common::ZMQWrapperApi::IContextSharedPtr context, int readLoopTimeoutMilliSeconds)
        : m_socketPath(socketAddress), m_readLoopTimeoutMilliSeconds(readLoopTimeoutMilliSeconds), m_context(context), m_socket(m_context->getSubscriber())
    {
        setSocketTimeout(m_readLoopTimeoutMilliSeconds);
        LOGINFO("Creating subscriber listening on socket address: " << m_socketPath);
    }

    Subscriber::~Subscriber()
    {
        LOGINFO("Stop not explicitly called, calling in destructor.");
        stop();
    }

    void Subscriber::subscribeToEvents()
    {

//        auto socket = m_context->getSubscriber();
//        socket->setTimeout(m_readLoopTimeoutMilliSeconds);

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
            // todo change to error message
            fs->makedirs(socketDir);
        }
//        socket->listen("ipc://" + m_socketPath);
        socketListen();

//            try
//            {
//                Common::FileSystem::filePermissions()->chown(m_socketPath, sophos::user(), sophos::group());
//                Common::FileSystem::filePermissions()->chmod(m_socketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
//            }
//            catch (Common::FileSystem::IFileSystemException& exception)
//            {
//                LOGERROR("Error setting up socket : "<< exception.what());
//            }

//        socket->subscribeTo("threatEvents");
        socketSubscribe("threatEvents");

        while (m_running)
        {
            std::cout << "waiting for event ..." << std::endl;
            try
            {
                auto data = socketRead();
                LOGINFO("received event");
                int index = 0;
                for (const auto& s : data)
                {
                    LOGINFO(index++ << ": " << s);
                }
            }
            catch(const Common::ZeroMQWrapper::IIPCException& exception)
            {
                int errnoFromSocketRead = errno;
                LOGINFO(std::to_string(errnoFromSocketRead));
                if (errnoFromSocketRead == EAGAIN)
                {
                    LOGDEBUG("Socket timeout");
                    // TODO
                    // Expected socket timeout. Using this so that our thread can exit and isn't blocked on read()
                    // for ever, we need to make sure this doesn't interfere with receiving messages though...
                }
                else
                {
                    LOGERROR("Unexpected exception from socket read: " << exception.what());
                    m_running = false;
                }
            }
            catch(const std::exception& exception)
            {
                m_running = false;
            }
        }
        unlink(m_socketPath.c_str());
    }

    void Subscriber::start()
    {
        LOGINFO("Starting Subscriber");
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
        LOGINFO(" thread has been stopped");
        auto fs = Common::FileSystem::fileSystem();
        if (fs->exists(m_socketPath))
        {
            LOGINFO("Waiting for subscriber socket to be removed");
            bool socketStillExists = Common::UtilityImpl::waitFor(1,0.1,[this, fs]() {
                return (!fs->exists(m_socketPath));
            });

            if (socketStillExists)
            {
                LOGERROR("Subscriber socket was not removed after Subscriber finished, deleting: " << m_socketPath);
                fs->removeFile(m_socketPath);
            }
            LOGINFO("Subscriber socket has been removed");
        }
        LOGINFO("Subscriber stopped");
    }

    void Subscriber::reset()
    {
        LOGINFO("Subscriber reset called");
        auto fs = Common::FileSystem::fileSystem();
        stop();
        start();
        bool socketExists = Common::UtilityImpl::waitFor(5,0.1,[this, fs]() {
          return (fs->exists(m_socketPath));
        });
        if (!socketExists)
        {
            LOGERROR("Socket was not created after starting subscriber thread");
        }
    }

    bool Subscriber::getRunningStatus()
    {
        return m_running;
    }
    void Subscriber::setSocketTimeout(int timeout)
    {
        m_socket->setTimeout(timeout);
    }
    std::vector<std::string> Subscriber::socketRead()
    {
        return m_socket->read();
    }
    void Subscriber::socketListen()
    {
        m_socket->listen("ipc://" + m_socketPath);
    }
    void Subscriber::socketSubscribe(const std::string& eventType) {
        m_socket->subscribeTo(eventType);
    }

}