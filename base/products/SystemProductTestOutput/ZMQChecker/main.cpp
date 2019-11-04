/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#include <Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h>
#include <Common/ZeroMQWrapper/IPoller.h>


#include <iostream>
#include <chrono>
#include <thread>
#include "Client.h"
#include "Server.h"

namespace
{
    //void SetTimeouts()
    constexpr int defaultTimeout = 1000;
    constexpr int connectTimeout = 5000;
    constexpr  char REQUESTER_SOCKET[] = "req";
    constexpr  char REPLIER_SOCKET[] = "rep";
    constexpr int SOCKET_TYPE_INTEX = 1;
    constexpr int IPC_PATH_INDEX = 2;

}

static int zmqchecker_main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "Expected 2 arguments, connection type ['req' | 'rep' ] and ipc path (e.g. '/tmp/test.ipc') " << std::endl;
    }

    std::string connectionType = argv[SOCKET_TYPE_INTEX];
    std::string ipc_path("ipc://");
    ipc_path = ipc_path + argv[IPC_PATH_INDEX];

    if( connectionType == REQUESTER_SOCKET)
    {
        Client client(ipc_path, connectTimeout);

        Common::ZeroMQWrapper::data_t data{"hello"};

        try
        {
            auto output = client.requestReply(data);
            for (auto &datum : output)
            {
                std::cout << "Requester data received: " << datum << std::endl;
            }
        }
        catch (const Common::ZeroMQWrapper::IIPCTimeoutException &e)
        {
            std::cout << "Error during requester write to ipc " << e.what() << std::endl;
        }
    }


    if( connectionType == REPLIER_SOCKET)
    {
        Server server(ipc_path, true);

        try
        {
            server.run();
            server.join();
        }
        catch (const Common::ZeroMQWrapper::IIPCTimeoutException& e)
        {
            std::cout << "Error during replier read from ipc socket " << e.what() << std::endl;
        }

    }
    return 0;
}

MAIN(zmqchecker_main(argc, argv))
