//
// Created by pair on 02/07/18.
//

#ifndef EVEREST_BASE_PLUGINCLIENT_H
#define EVEREST_BASE_PLUGINCLIENT_H

#include "Common/ZeroMQWrapper/IReadWrite.h"
#include "MessageBuilder.h"

namespace Common
{
    namespace PluginApiImpl
    {
        class PluginClient
        {
        public:
            PluginClient(const std::string & pluginID,  std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite );

            void sendRequest( DataMessage );
            //std::string

        private:
            MessageBuilder m_messageBuilder;
            std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> m_socket;
        };

    }
}
#endif //EVEREST_BASE_PLUGINCLIENT_H
