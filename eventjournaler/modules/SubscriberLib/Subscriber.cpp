/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Subscriber.h"

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>

#include <iostream>
#include <unistd.h>

namespace SubscriberLib
{
    void Subscriber::subscribeToEvents()
    {
//        uid_t owner = 0;
//        gid_t group = 0;

//        if (!user.empty())
//        {
//            if (getUserGroupID(user, owner, group))
//            {
//                std::cout << user << " " << owner << ":" << group << std::endl;
//            }
//        }
        auto context = Common::ZMQWrapperApi::createContext();
        auto socket = context->getSubscriber();
//        assert(socket.get());

        socket->listen("ipc://" + m_socketPath);

//        if (chmod(socket_path.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)
//        {
//            std::cout << "failed to change permissions of \"" << socket_path << "\"" << std::endl;
//        }

//        if (owner && group)
//        {
//            if (chown(socket_path.c_str(), owner, group) != 0)
//            {
//                std::cout << "failed to change ownership of \"" << socket_path << "\"" << std::endl;
//            }
//        }

        socket->subscribeTo("threatEvents");

//        for (int i = 0; i < count; i++)
//        {
        std::cout << "waiting for event ..." << std::endl;
        auto data = socket->read();
        std::cout << "received event" << std::endl;
        int index = 0;
        for (const auto& s : data)
        {
            std::cout << index++ << ": " << s << std::endl;
        }
        std::cout << std::endl;
//        }

        unlink(m_socketPath.c_str());
    }
}