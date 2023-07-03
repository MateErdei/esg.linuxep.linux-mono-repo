/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "TestListener.h"

#include "modules/Common/Reactor/IReactor.h"
#include "modules/Common/ZMQWrapperApi/IContext.h"

#include <string>
#include <vector>

class FakeServer
{
public:
    FakeServer(const std::string& socketAddress, bool captureSignals);
    void run(Common::ZMQWrapperApi::IContext& iContext);
    void join();

private:
    std::string m_socketAddress;
    std::unique_ptr<Common::Reactor::IReactor> m_reactor;
    std::unique_ptr<TestListener> m_testListener;
    std::unique_ptr<Common::Reactor::IShutdownListener> m_shutdownListener;
    bool m_captureSignals;
};
