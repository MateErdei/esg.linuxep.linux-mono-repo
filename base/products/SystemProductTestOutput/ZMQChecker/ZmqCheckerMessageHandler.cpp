/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ZmqCheckerMessageHandler.h"
#include <iostream>
#include <thread>

namespace zmqchecker
{
    const int connectionTimeout = 5000;
    ZmqCheckerMessageHandler::ZmqCheckerMessageHandler(
            std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> socketReplier) : m_socketReplier(std::move(socketReplier)) {}

    void ZmqCheckerMessageHandler::messageHandler(Common::ZeroMQWrapper::IReadable::data_t processData)
    {
        for(auto& datum : processData)
        {
            std::cout << "Received : " << datum << std::endl;
            if (datum == "ignore")
            {
                std::cout << "ignoring requests for : " << connectionTimeout*3 << " milliseconds" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(connectionTimeout*3));
            }
            else
            {
                Common::ZeroMQWrapper::data_t data{"world"};
                m_socketReplier->write(data);
            }
        }

    }
}