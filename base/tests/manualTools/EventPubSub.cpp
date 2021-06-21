/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>


// Listen for event from AV plugin
//  EventPubSub -s /opt/sophos-spl/plugins/av/var/threatEventPublisherSocketPath -u sophos-spl-av listen
//
// Generate AV event
//  EventPubSub -s /opt/sophos-spl/plugins/av/var/threatEventPublisherSocketPath send

void printUsageAndExit(const std::string name)
{
    std::cout << "usage: " << name << " [-c <count> -s <socket path> -u <user>] send | listen" << std::endl;
    exit(EXIT_FAILURE);
}

bool getUserGroupID(const std::string user, uid_t& owner_id, gid_t& group_id)
{
    struct passwd pwd;
    struct passwd* result = nullptr;
    char buffer[4 * 1024];
    int error = getpwnam_r(user.c_str(), &pwd, buffer, sizeof(buffer), &result);
    if (error)
    {
        std::cerr << "failed to get user/group ID for \"" << user << "\" - error " << error << std::endl;
        return false;
    }
    if (!result)
    {
        std::cout << "user \"" << user << "\" not found" << std::endl;
        return false;
    }

    owner_id = result->pw_uid;
    group_id = result->pw_gid;

    return true;
}


int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsageAndExit(argv[0]);
    }

    std::string socket_path = "event.sock";
    std::string user;
    int count = 1;
    int opt = 0;

    while ((opt = getopt(argc, argv, "c:s:u:")) != -1)
    {
        switch (opt)
        {
            case 'c':
                count = atoi(optarg);
                break;
            case 's':
                socket_path = optarg;
                break;
            case 'u':
                user = optarg;
                break;
            default:
                printUsageAndExit(argv[0]);
        }
    }

    std::string command = argv[optind];


    auto context = Common::ZMQWrapperApi::createContext();
    assert(context.get());

    if (command.compare("send") == 0)
    {
        const std::string threat_detected_json = R"({"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"})";

        auto socket = context->getPublisher();
        assert(socket.get());

        socket->connect("ipc://" + socket_path);
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for connection

        for (int i = 0; i < count; i++)
        {
            std::cout << "sending event" << std::endl;
            socket->write({ "threatEvents", threat_detected_json });
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    else if (command.compare("listen") == 0)
    {
        uid_t owner = 0;
        gid_t group = 0;

        if (!user.empty())
        {
            if (getUserGroupID(user, owner, group))
            {
                std::cout << user << " " << owner << ":" << group << std::endl;
            }
        }

        auto socket = context->getSubscriber();
        assert(socket.get());

        socket->listen("ipc://" + socket_path);

        if (chmod(socket_path.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)
        {
            std::cout << "failed to change permissions of \"" << socket_path << "\"" << std::endl;
        }

        if (owner && group)
        {
            if (chown(socket_path.c_str(), owner, group) != 0)
            {
                std::cout << "failed to change ownership of \"" << socket_path << "\"" << std::endl;
            }
        }

        socket->subscribeTo("threatEvents");

        for (int i = 0; i < count; i++)
        {
            std::cout << "waiting for event ..." << std::endl;
            auto data = socket->read();
            std::cout << "received event" << std::endl;
            int index = 0;
            for (const auto& s : data)
            {
                std::cout << index++ << ": " << s << std::endl;
            }
            std::cout << std::endl;
        }

        unlink(socket_path.c_str());
    }
    else
    {
        printUsageAndExit(argv[0]);
    }

    return EXIT_SUCCESS;
}
