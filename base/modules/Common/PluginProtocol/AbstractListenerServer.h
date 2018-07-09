/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_ABSTRACTSERVER_H
#define EVEREST_BASE_ABSTRACTSERVER_H

#include "Common/PluginProtocol/Protocol.h"

#include "Common/ZeroMQWrapper/IReadable.h"
#include "Common/Reactor/IReactor.h"
#include "Common/Reactor/ICallbackListener.h"

#include <memory>

namespace Common
{
    namespace PluginProtocol
    {
        using namespace PluginProtocol;

        class AbstractListenerServer :
                public virtual Common::Reactor::ICallbackListener,
                public virtual Common::Reactor::IShutdownListener
        {
        public:
            AbstractListenerServer(std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite);
            virtual ~AbstractListenerServer();
            void start() ;
            void stop() ;

        private:
            virtual DataMessage process(const DataMessage & request) const = 0;
            virtual void onShutdownRequested() = 0;

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
//            message.Payload = OK;
//            return message;
//        }
//    }
//};

#endif //EVEREST_BASE_ABSTRACTSERVER_H
