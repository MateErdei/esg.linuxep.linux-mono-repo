//
// Created by pair on 01/11/19.
//

#include "ZmqCheckerMessageHandler.h"
#include <iostream>

ZmqCheckerMessageHandler::ZmqCheckerMessageHandler(
        std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> socketReplier, std::function<void(void)> function) : m_socketReplier(std::move(socketReplier)), m_stopFunction(std::move(function)) {}

void ZmqCheckerMessageHandler::messageHandler(Common::ZeroMQWrapper::IReadable::data_t processData)
{
    std::cout << "Received : " << processData[0] << std::endl;
    Common::ZeroMQWrapper::data_t data{"world"};
    m_socketReplier->write(data);
    //Stop the reactor after the first reply
    m_stopFunction();
}