/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PLUGINCALLBACKHANDLER_H
#define EVEREST_BASE_PLUGINCALLBACKHANDLER_H

#include <Common/ZeroMQWrapper/IReadWrite.h>
#include "Common/PluginProtocol/AbstractListenerServer.h"
#include "IPluginCallback.h"
#include "Common/PluginProtocol/MessageBuilder.h"

namespace Common
{

    namespace PluginApiImpl
    {
        class PluginCallBackHandler : public AbstractListenerServer
        {
         public:
            PluginCallBackHandler( std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                                   std::shared_ptr<Common::PluginApi::IPluginCallback> pluginCallback);


        private:
            DataMessage process(const DataMessage & request) const override ;
            void onShutdownRequested() override ;

            MessageBuilder m_messageBuilder;
            std::shared_ptr<Common::PluginApi::IPluginCallback> m_pluginCallback;

        };
    }
}



#endif //EVEREST_BASE_PLUGINCALLBACKHANDLER_H
