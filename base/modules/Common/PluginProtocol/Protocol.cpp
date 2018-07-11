/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <sstream>
#include <Common/PluginProtocol/IProtocolSerializer.h>
#include "Protocol.h"
#include "ProtocolSerializer.h"

namespace Common
{
    namespace PluginProtocol
    {
        //using DataMessage = Common::PluginProtocol::DataMessage;
        Protocol::Protocol(std::unique_ptr<PluginProtocol::ProtocolSerializerFactory> protocolFactory)
        : m_protocolFactory(std::move(protocolFactory))
        {

        }

        const data_t Protocol::serialize(const Common::PluginProtocol::DataMessage &dataMessage)const
        {
            auto serializer = m_protocolFactory->createProtocolSerializer(dataMessage.ProtocolVersion);

            return serializer->serialize(dataMessage);
        }

        const Common::PluginProtocol::DataMessage Protocol::deserialize(const data_t &data) const
        {
            std::string protocolversion;
            if (!data.empty())
            {
                protocolversion = data[0];
            }

            auto serializer = m_protocolFactory->createProtocolSerializer(protocolversion);

            return serializer->deserialize(data);
        }


    }
}