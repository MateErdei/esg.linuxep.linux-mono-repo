/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/Common/Reactor/ICallbackListener.h"
#include "modules/Common/ZeroMQWrapper/ISocketReplier.h"

class TestListener : public Common::Reactor::ICallbackListener
{
public:
    TestListener(std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> socketReplier);

    void messageHandler(Common::ZeroMQWrapper::IReadable::data_t) override;

private:
    std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> m_socketReplier;
};
