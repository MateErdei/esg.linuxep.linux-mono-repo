/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_ABSTRACTSERVER_H
#define EVEREST_BASE_ABSTRACTSERVER_H

#include "PluginApi/IListenerServer.h"

#include "Protocol.h"

#include "Common/ZeroMQWrapper/IReadable.h"
#include "Common/Reactor/IReactor.h"
#include "Common/Reactor/ICallbackListener.h"

#include <memory>

namespace Common
{
    namespace PluginApiImpl
    {
        using namespace PluginApi;

        class AbstractListenerServer : public IListenerServer,
                public Common::Reactor::ICallbackListener,
                public Common::Reactor::IShutdownListener
        {
        public:
            AbstractListenerServer(std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite);
            void start() override;
            void stop() override;
// REPLY
        private:
            void messageHandler(Common::ZeroMQWrapper::IReadable::data_t request ) override ;
            void notifyShutdownRequested() override ;
            std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> m_ireadWrite;
            std::unique_ptr<Common::Reactor::IReactor> m_reactor;

        };
    }
}


//
//PluginCallbackHandler : public AbstractListenerServer
//
//        PluginAdapterCallbacks
//        ireadwriter...
//
//
//{
//
//    message reply ( message )
//    {
//        if ( message.command == Status)
//        {
//            m_callbackHandler->setStatus(xx, xx);
//            message.payload = OK;
//            return message;
//        }
//    }
//};

#endif //EVEREST_BASE_ABSTRACTSERVER_H
