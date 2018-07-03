//
// Created by pair on 27/06/18.
//

#include <sstream>
#include <Common/PluginApi/IProtocolSerializer.h>
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
            Common::PluginApi::DataMessage message;
            if ( data.size() == 0 )
            {
                message.Error = "Bad formed message";
                return message;
            }

            auto serializer = m_protocolFactory->createProtocolSerializer(data[0]);

            if(!serializer)
            {
                message.Error = "Protocol Not supported";
                return message;
            }

            return serializer->deserialize(data);
        }


    }
}