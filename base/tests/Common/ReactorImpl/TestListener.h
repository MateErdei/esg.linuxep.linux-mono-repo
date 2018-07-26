//
// Created by pair on 26/06/18.
//

#pragma once


#include "Common/Reactor/ICallbackListener.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"

class TestListener : public Common::Reactor::ICallbackListener
{
public:
    TestListener(std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> socketReplier);

    void messageHandler(Common::ZeroMQWrapper::IReadable::data_t ) override;
private:
    std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> m_socketReplier;
};



