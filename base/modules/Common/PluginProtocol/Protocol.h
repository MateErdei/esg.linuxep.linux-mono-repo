/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PROTOCOL_H
#define EVEREST_BASE_PROTOCOL_H

#include "Common/PluginProtocol/DataMessage.h"
#include "Common/PluginProtocol/ProtocolSerializerFactory.h"
#include "ProtocolSerializerFactory.h"

namespace Common
{
    namespace PluginApiImpl
    {
        using data_t = std::vector<std::string>;

        class Protocol
        {
        public:
            Protocol(
                    std::unique_ptr<PluginApiImpl::ProtocolSerializerFactory> protocolFactory =
                            std::unique_ptr<Common::PluginApiImpl::ProtocolSerializerFactory>(new Common::PluginApiImpl::ProtocolSerializerFactory()));
            const data_t serialize(const Common::PluginApi::DataMessage &data)const;

            const Common::PluginApi::DataMessage deserialize(const data_t &serializedData);
        private:
            std::unique_ptr<PluginApiImpl::ProtocolSerializerFactory> m_protocolFactory;
        };
    }
}
#endif //EVEREST_BASE_PROTOCOL_H
