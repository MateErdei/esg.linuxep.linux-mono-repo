//
// Created by pair on 26/06/18.
//

#include <sstream>
#include "TestListener.h"

TestListener::TestListener(std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> socketReplier)
    : m_socketReplier(std::move(socketReplier))
{
}


void TestListener::process(std::vector<std::string> processData)
{
    std::string & command = processData[0];
    if(command == "echo")
    {
        m_socketReplier->write(processData);
    }
    else if (command == "concat")
    {
        std::stringstream ss;
        for( size_t i = 1; i< processData.size(); i++)
        {
            ss << processData[i];
        }

        m_socketReplier->write(std::vector<std::string>{command, ss.str()});
    }
    else if (command == "quit")
    {
        m_socketReplier->write(std::vector<std::string>{command});
        throw Common::Reactor::StopReactorRequest();
    }
    else
    {
        m_socketReplier->write(std::vector<std::string>{command,"Command not supported"});

    }

}
