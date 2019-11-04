/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Reactor/ICallbackListener.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include <functional>

class ZmqCheckerMessageHandler : public Common::Reactor::ICallbackListener
{
public:
    ZmqCheckerMessageHandler(std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> socketReplier, std::function<void(void)> function);

    void messageHandler(Common::ZeroMQWrapper::IReadable::data_t) override;

private:
    std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> m_socketReplier;
    std::function<void()> m_stopFunction;
};
