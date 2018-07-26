//
// Created by pair on 26/06/18.
//

#pragma once


#include "Common/Reactor/IReactor.h"
#include "TestListener.h"
#include "Common/ZeroMQWrapper/IContext.h"
#include <string>
#include <vector>

class FakeServer
{
public:
    FakeServer(const std::string & socketAddress, bool captureSignals);
    void run( Common::ZeroMQWrapper::IContext& iContext);
    void join();

private:
    std::string m_socketAddress;
    std::unique_ptr<Common::Reactor::IReactor> m_reactor;
    std::unique_ptr<TestListener> m_testListener;
    std::unique_ptr<Common::Reactor::IShutdownListener> m_shutdownListener;
    bool m_captureSignals;

};



