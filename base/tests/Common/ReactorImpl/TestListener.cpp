/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TestListener.h"

#include <sstream>
using data_t = Common::ZeroMQWrapper::IReadable::data_t;
TestListener::TestListener(std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> socketReplier) :
    m_socketReplier(std::move(socketReplier))
{
}

void TestListener::messageHandler(Common::ZeroMQWrapper::IReadable::data_t processData)
{
    std::string& command = processData[0];
    if (command == "echo")
    {
        m_socketReplier->write(processData);
    }
    else if (command == "concat")
    {
        std::stringstream ss;
        for (size_t i = 1; i < processData.size(); i++)
        {
            ss << processData[i];
        }

        m_socketReplier->write(data_t{ command, ss.str() });
    }
    else if (command == "quit")
    {
        m_socketReplier->write(data_t{ command });
        throw Common::Reactor::StopReactorRequestException();
    }
    else
    {
        m_socketReplier->write(data_t{ command, "Command not supported" });
    }
}
