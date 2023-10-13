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

#include <sstream>
#include <fstream>
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
    std::cout << "usage: " << name << " [-c <count> -s <socket path> -u <user> -d <data> -f <file path of data>] send | listen" << std::endl;
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

    std::string socketPath = "event.sock";
    std::string user;
    std::string dataString;
    std::string filepath;
    bool customDataSet = false;
    int count = 1;
    int opt = 0;

    while ((opt = getopt(argc, argv, "c:s:u:d:f:")) != -1)
    {
        switch (opt)
        {
            case 'c':
                count = atoi(optarg);
                break;
            case 's':
                socketPath = optarg;
                break;
            case 'u':
                user = optarg;
                break;
            case 'd':
                dataString = optarg;
                customDataSet = true;
                break;
            case 'f':
                filepath = optarg;
                customDataSet = true;
                break;
            default:
                printUsageAndExit(argv[0]);
        }
    }

    std::string command = argv[optind];

    auto context = Common::ZMQWrapperApi::createContext();
    assert(context.get());

    if (command == "send")
    {
        if (!customDataSet)
        {
            dataString = R"({"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"})";
        }
        if (!filepath.empty())
        {
            std::stringstream data;
            std::string contents;

            std::ifstream dataFile(filepath);
            while (getline (dataFile, contents))
            {
                data << contents;
            }
            dataString = data.str();
        }
        auto socket = context->getPublisher();
        assert(socket.get());

        socket->connect("ipc://" + socketPath);
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for connection

        for (int i = 0; i < count; i++)
        {
            std::cout << "sending event" << std::endl;
            socket->write({ "threatEvents", dataString });
            std::this_thread::sleep_for(std::chrono::milliseconds (100));
        }
    }
    else if (command == "listen")
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

        socket->listen("ipc://" + socketPath);

        if (chmod(socketPath.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)
        {
            std::cout << "failed to change permissions of \"" << socketPath << "\"" << std::endl;
        }

        if (owner && group)
        {
            if (chown(socketPath.c_str(), owner, group) != 0)
            {
                std::cout << "failed to change ownership of \"" << socketPath << "\"" << std::endl;
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

        unlink(socketPath.c_str());
    }
    else
    {
        printUsageAndExit(argv[0]);
    }

    return EXIT_SUCCESS;
}
