//
// Created by pair on 27/06/18.
//

#ifndef EVEREST_BASE_PROTOCOL_H
#define EVEREST_BASE_PROTOCOL_H

#include "DataMessage.h"
#include "Common/PluginApi/IProtocolSerializerFactory.h"
#include "Common/PluginApiImpl/ProtocolSerializerFactory.h"

namespace Common
{
    namespace PluginApiImpl
    {
        using data_t = std::vector<std::string>;

        class Protocol
        {
        public:
            Protocol(
                    std::unique_ptr<PluginApi::IProtocolSerializerFactory> protocolFactory =
                            std::unique_ptr<Common::PluginApi::IProtocolSerializerFactory>(new Common::PluginApiImpl::ProtocolSerializerFactory()));
            const data_t serialize(const Common::PluginApi::DataMessage &data)const;

            const Common::PluginApi::DataMessage deserialize(const data_t &serializedData);
        private:
            std::unique_ptr<PluginApi::IProtocolSerializerFactory> m_protocolFactory;
        };
    }
}
#endif //EVEREST_BASE_PROTOCOL_H
