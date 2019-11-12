/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Client.h"

#include "Common/ZeroMQWrapper/ISocketRequester.h"

#include <iostream>
namespace zmqchecker
{
    Client::~Client() = default;

    Client::Client(const std::string& address, int timeout) : m_iContextSharedPtr(Common::ZMQWrapperApi::createContext())
    {
        m_socketRequester = m_iContextSharedPtr->getRequester();
        if (timeout != -1)
        {
            m_socketRequester->setConnectionTimeout(timeout);
        }
        std::cout << "Connecting to address: " << address << std::endl;
        m_socketRequester->connect(address);
    }

    Common::ZeroMQWrapper::IReadable::data_t Client::requestReply(
            const Common::ZeroMQWrapper::IReadable::data_t& request)
    {
        std::cout << "Writting request " << std::endl;
        m_socketRequester->write(request);
        std::cout << "awaiting response " << std::endl;
        return m_socketRequester->read();
    }
}