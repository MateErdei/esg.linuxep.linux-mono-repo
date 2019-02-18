/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Protocol.h"

#include "ProtocolSerializer.h"

#include <Common/PluginProtocol/IProtocolSerializer.h>

#include <sstream>

namespace Common
{
    namespace PluginProtocol
    {
        // using DataMessage = Common::PluginProtocol::DataMessage;
        Protocol::Protocol(std::unique_ptr<PluginProtocol::ProtocolSerializerFactory> protocolFactory) :
            m_protocolFactory(std::move(protocolFactory))
        {
        }

        const data_t Protocol::serialize(const Common::PluginProtocol::DataMessage& dataMessage) const
        {
            auto serializer = m_protocolFactory->createProtocolSerializer();
            return serializer->serialize(dataMessage);
        }

        const Common::PluginProtocol::DataMessage Protocol::deserialize(const data_t& data) const
        {
            auto serializer = m_protocolFactory->createProtocolSerializer();
            return serializer->deserialize(data);
        }

    } // namespace PluginProtocol
} // namespace Common