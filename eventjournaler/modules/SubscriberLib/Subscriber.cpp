/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Subscriber.h"

#include "Logger.h"

#include <Common/FileSystem/IFilePermissions.h>
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
        assert(socket.get());
        try
        {
            socket->listen("ipc://" + m_socketPath);

            try
            {
                Common::FileSystem::filePermissions()->chown(m_socketPath, sophos::user(), sophos::group());
                Common::FileSystem::filePermissions()->chmod(m_socketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            }
            catch (Common::FileSystem::IFileSystemException& exception)
            {
                LOGERROR("Error setting up socket : "<< exception.what());
            }

        socket->subscribeTo("threatEvents");

            //        for (int i = 0; i < count; i++)
            //        {
            std::cout << "waiting for event ..." << std::endl;
            auto data = socket->read();
            LOGINFO("received event");
            int index = 0;
            for (const auto& s : data)
            {
                std::cout << index++ << ": " << s << std::endl;
            }
            std::cout << std::endl;
            //        }
        }
        catch (std::exception& exception)
        {
            LOGERROR("Exception thrown when trying to start subscriber: "<< exception.what());
            LOGERROR(strerror(errno));
        }

        unlink(m_socketPath.c_str());
    }
}