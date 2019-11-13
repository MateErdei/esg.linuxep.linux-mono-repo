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
    constexpr int connectTimeout = 5000;
    constexpr  char REQUESTER_SOCKET[] = "req";
    constexpr  char REPLIER_SOCKET[] = "rep";
    constexpr  char REPLIER_NOREPLY_SOCKET[] = "rep-noreply";
    constexpr  char REQUEST_NO_READ_SOCKET[] = "req-noread";
    constexpr int SOCKET_TYPE_INTEX = 1;
    constexpr int IPC_PATH_INDEX = 2;

}

using namespace zmqchecker;
static int zmqchecker_main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "Requires 2 arguments, connection type ['" << REQUESTER_SOCKET <<"', '" << REPLIER_SOCKET << "', '"
                                                                         << REPLIER_NOREPLY_SOCKET <<"', '"<< REQUEST_NO_READ_SOCKET <<"']. And ipc path (e.g. '/tmp/test.ipc') " << std::endl;
        return 1;
    }

    std::string connectionType = argv[SOCKET_TYPE_INTEX];
    std::string ipc_path("ipc://");
    ipc_path = ipc_path + argv[IPC_PATH_INDEX];

    if( connectionType == REQUESTER_SOCKET || connectionType == REQUEST_NO_READ_SOCKET)
    {
        Client client(ipc_path, connectTimeout);

        std::string command("hello");
        Common::ZeroMQWrapper::data_t data{command};
        try
        {
            Common::ZeroMQWrapper::IReadable::data_t output;
            if(connectionType == REQUEST_NO_READ_SOCKET)
            {
                output  = client.requestReply(data, true);
            }
            else
            {
                output = client.requestReply(data);
            }
            for (auto &datum : output)
            {
                std::cout << "Requester data received: " << datum << std::endl;
            }
        }
        catch (const Common::ZeroMQWrapper::IIPCTimeoutException &e)
        {
            std::cout << "Error during requester write to ipc " << e.what() << std::endl;
            return 1;
        }
        catch (const Common::ZeroMQWrapperImpl::ZeroMQWrapperException& zre)
        {
            std::cout << "Requester ZMQ exception " << zre.what() << std::endl;
            return 2;
        }

    }
    else if( connectionType == REPLIER_SOCKET || connectionType == REPLIER_NOREPLY_SOCKET)
    {

        bool ignoreRequests = (connectionType == REPLIER_NOREPLY_SOCKET);
        Server server(ipc_path, true, ignoreRequests);
        try
        {
            server.run();
            std::cout << "reactor started and now listening" << std::endl;
            server.join();
        }
        catch (const Common::ZeroMQWrapper::IIPCTimeoutException& e)
        {
            std::cout << "Error during replier read from ipc socket " << e.what() << std::endl;
        }
        catch (const Common::ZeroMQWrapperImpl::ZeroMQWrapperException& zre)
        {
            std::cout << "Replier ZMQ error :" << zre.what() << std::endl;
            return 2;
        }

    }
    else
    {
        std::cout << "Unknown Connection Type Requested" << "must be one of [ '" << REQUESTER_SOCKET <<"', '" << REPLIER_SOCKET << "', '"
                << REPLIER_NOREPLY_SOCKET <<"', '"<< REQUEST_NO_READ_SOCKET <<"'] "<< std::endl;
    }
    return 0;
}

MAIN(zmqchecker_main(argc, argv))
