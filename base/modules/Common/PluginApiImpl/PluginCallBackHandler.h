/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PLUGINCALLBACKHANDLER_H
#define EVEREST_BASE_PLUGINCALLBACKHANDLER_H

#include <Common/ZeroMQWrapper/IReadWrite.h>
#include "Common/PluginProtocol/AbstractListenerServer.h"
#include "Common/PluginApi/IPluginCallbackApi.h"
#include "Common/PluginProtocol/MessageBuilder.h"

namespace Common
{

    namespace PluginApiImpl
    {
        class PluginCallBackHandler : public Common::PluginProtocol::AbstractListenerServer
        {
         public:
            PluginCallBackHandler(const std::string &pluginName,
                                  std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                                  std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback);


        private:
            Common::PluginProtocol::DataMessage process(const Common::PluginProtocol::DataMessage & request) const override ;
            void onShutdownRequested() override ;

            Common::PluginProtocol::MessageBuilder m_messageBuilder;
            std::shared_ptr<Common::PluginApi::IPluginCallbackApi> m_pluginCallback;

        };
    }
}



#endif //EVEREST_BASE_PLUGINCALLBACKHANDLER_H
