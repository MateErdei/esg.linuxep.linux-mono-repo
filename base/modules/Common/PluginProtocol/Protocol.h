/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ProtocolSerializerFactory.h"

#include "Common/PluginProtocol/DataMessage.h"
#include "Common/PluginProtocol/ProtocolSerializerFactory.h"

namespace Common
{
    namespace PluginProtocol
    {
        using data_t = Common::ZeroMQWrapper::data_t;

        class Protocol
        {
        public:
            explicit Protocol(
                std::unique_ptr<PluginProtocol::ProtocolSerializerFactory> protocolFactory =
                    std::unique_ptr<Common::PluginProtocol::ProtocolSerializerFactory>(
                        new Common::PluginProtocol::ProtocolSerializerFactory()));
            const data_t serialize(const Common::PluginProtocol::DataMessage& data) const;

            const Common::PluginProtocol::DataMessage deserialize(const data_t& serializedData) const;

        private:
            std::unique_ptr<PluginProtocol::ProtocolSerializerFactory> m_protocolFactory;
        };
    } // namespace PluginProtocol
} // namespace Common
