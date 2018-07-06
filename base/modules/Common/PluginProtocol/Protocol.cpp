/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <sstream>
#include <Common/PluginProtocol/IProtocolSerializer.h>
#include "Protocol.h"
#include "ProtocolSerializerFactory.h"
#include "ProtocolSerializer.h"

namespace Common
{
    namespace PluginApiImpl
    {
        //using DataMessage = Common::PluginApi::DataMessage;
        Protocol::Protocol(std::unique_ptr<PluginApi::IProtocolSerializerFactory> protocolFactory)
        : m_protocolFactory(std::move(protocolFactory))
        {

        }

        const data_t Protocol::serialize(const Common::PluginApi::DataMessage &dataMessage)const
        {
            auto serializer = m_protocolFactory->createProtocolSerializer(dataMessage.ProtocolVersion);

            return serializer->serialize(dataMessage);
        }

        const Common::PluginApi::DataMessage Protocol::deserialize(const data_t & data)
        {
            std::string protocolversion;
            if ( data.size() !=0 )
            {
                protocolversion = data[0];
            }

            auto serializer = m_protocolFactory->createProtocolSerializer(protocolversion);

            return serializer->deserialize(data);
        }


    }
}