//
// Created by pair on 26/06/18.
//

#ifndef EVEREST_BASE_TESTLISTENER_H
#define EVEREST_BASE_TESTLISTENER_H

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


#endif //EVEREST_BASE_TESTLISTENER_H
