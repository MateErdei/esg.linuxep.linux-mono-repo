/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Subscriber.h"

#include "Logger.h"

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/ProjectNames.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <sys/stat.h>

#include <cstring>
#include <iostream>
#include <unistd.h>

namespace SubscriberLib
{
    void Subscriber::subscribeToEvents()
    {
        auto context = Common::ZMQWrapperApi::createContext();
        auto socket = context->getSubscriber();
        socket->setTimeout(5000);
//        assert(socket.get());
//        try
//        {
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
                fs->makedirs(socketDir);
            }
            socket->listen("ipc://" + m_socketPath);

//            try
//            {
//                Common::FileSystem::filePermissions()->chown(m_socketPath, sophos::user(), sophos::group());
//                Common::FileSystem::filePermissions()->chmod(m_socketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
//            }
//            catch (Common::FileSystem::IFileSystemException& exception)
//            {
//                LOGERROR("Error setting up socket : "<< exception.what());
//            }

            socket->subscribeTo("threatEvents");

            //        for (int i = 0; i < count; i++)
            //        {
            while (m_running)
            {
                std::cout << "waiting for event ..." << std::endl;
                try
                {
                    auto data = socket->read();
                    LOGINFO("received event");
                    int index = 0;
                    for (const auto& s : data)
                    {
                        LOGINFO(index++ << ": " << s);
                    }
                }
                catch(const std::exception& exception)
                {
                    int errnoFromSocketRead = errno;
//                    std::cout << std::to_string(errnoFromSocketRead) << std::endl;
                    LOGINFO(std::to_string(errnoFromSocketRead));

                    if (errnoFromSocketRead == EAGAIN)
                    {
//                        std::cout << "Socket timeout" << std::endl;
                        LOGINFO("Socket timeout");

                        // TODO
                        // Expected socket timeout. Using this so that our thread can exit and isn't blocked on read()
                        // for ever, we need to make sure this doesn't interfere with receiving messages though...
                    }
                    else
                    {
                        // TODO how to handle this?
                        LOGINFO("Unexpected exception from socket read: " << exception.what());
                        throw exception;
                    }
                }

            }
//        }
//        catch (std::exception& exception)
//        {
//            LOGERROR("Exception thrown when trying to start subscriber: "<< exception.what());
//            LOGERROR(strerror(errno));
//        }

        unlink(m_socketPath.c_str());
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
        auto fs = Common::FileSystem::fileSystem();
        if (fs->exists(m_socketPath))
        {
            LOGINFO("Waiting for subscriber socket to be removed");
            bool socketStillExists = waitFor(1,0.1,[this, fs]() {
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

    void Subscriber::start()
    {
        LOGINFO("Starting Subscriber");
        m_running = true;
        m_runnerThread = std::make_unique<std::thread>(std::thread([this] { subscribeToEvents(); }));
        LOGINFO("Subscriber started");
    }

    Subscriber::Subscriber(const std::string& socketAddress)
    : m_socketPath(socketAddress)
    {
        LOGINFO("Creating subscriber listening on socket address: " << m_socketPath);
    }

    Subscriber::~Subscriber()
    {
        LOGINFO("Stop not explicitly called, calling in destructor.");
        stop();
    }

    bool Subscriber::waitFor(
        double timeToWaitInSeconds,
        double intervalInSeconds,
        std::function<bool()> conditionFunction)
    {
        if (conditionFunction())
        {
            return true;
        }
        else
        {
            unsigned int intervalMicroSeconds = intervalInSeconds * 1000000;
            double maxNumberOfSleeps = timeToWaitInSeconds / intervalInSeconds;
            double sleepCounter = 0;
            while (!conditionFunction() && sleepCounter++ < maxNumberOfSleeps)
            {
                usleep(intervalMicroSeconds);
            }
        }
        return false;
    }
}