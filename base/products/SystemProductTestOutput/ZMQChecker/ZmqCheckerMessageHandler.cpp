/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ZmqCheckerMessageHandler.h"

#include <iostream>
#include <thread>

namespace zmqchecker
{
    const int connectionTimeout = 3000;
    ZmqCheckerMessageHandler::ZmqCheckerMessageHandler(
        std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> socketReplier,
        bool ignoreRequests) :
        m_socketReplier(std::move(socketReplier)), m_ignoreRequests(ignoreRequests)
    {
    }

    void ZmqCheckerMessageHandler::messageHandler(Common::ZeroMQWrapper::IReadable::data_t processData)
    {
        for (auto& datum : processData)
        {
            std::cout << "Received : " << datum << std::endl;
            if (m_ignoreRequests)
            {
                std::cout << "ignoring requests for : " << connectionTimeout * 3 << " milliseconds" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(connectionTimeout * 3));
            }
            else
            {
                Common::ZeroMQWrapper::data_t data{ "world" };
                m_socketReplier->write(data);
            }
        }
    }
} // namespace zmqchecker