/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProtocolSerializer.h"
#include "ProtocolSerializerFactory.h"

#include <iostream>

#define LOGERROR(x) std::cerr << x << '\n'
#define LOGDEBUG(x) std::cerr << x << '\n'
#define LOGSUPPORT(x) std::cerr << x << '\n'

namespace Common
{
    namespace PluginProtocol
    {
        ProtocolSerializer::ProtocolSerializer()
        : m_supportedProtocolVersion(Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion)
        {

        }

        const data_t ProtocolSerializer::serialize(const Common::PluginProtocol::DataMessage &dataMessage)const
        {
            data_t data;
            data.push_back(dataMessage.ProtocolVersion);
            data.push_back(dataMessage.ApplicationId);
            data.push_back(PluginProtocol::SerializeCommand(dataMessage.Command));
            data.push_back(dataMessage.MessageId);

            if (dataMessage.Error.empty())
            {
                data.insert(data.begin()+data.size(),dataMessage.Payload.begin(), dataMessage.Payload.end());
            }
            else
            {
                data.push_back(ProtocolSerializerFactory::ProtocolErrorMark);
                data.push_back(dataMessage.Error);
            }

            return data;
        }

        const Common::PluginProtocol::DataMessage ProtocolSerializer::deserialize(const data_t &serializedData)
        {
            Common::PluginProtocol::DataMessage message;
            if ( serializedData.size() < 4 )
            {
                message = createDefaultErrorMessage();
                message.Error = "Bad formed message";
                LOGDEBUG("Protocol Serializer error: " << message.Error);
                return message;
            }

            if(m_supportedProtocolVersion != serializedData[0])
            {
                message = createDefaultErrorMessage();
                message.Error = "Protocol not supported";
                LOGDEBUG("Protocol Serializer error: " << message.Error);
                return message;
            }

            message.ProtocolVersion = serializedData[0];
            message.ApplicationId = serializedData[1];
            message.Command = PluginProtocol::DeserializeCommand(serializedData[2]);

            if(message.Command == PluginProtocol::Commands::UNKNOWN)
            {
                message.Error = "Invalid request";
                LOGDEBUG("Protocol Serializer error: " << message.Error);
            }

            message.MessageId = serializedData[3];
            if ( serializedData.size() > 4)
            {
                if ( serializedData[4] == ProtocolSerializerFactory::ProtocolErrorMark)
                {
                    if ( serializedData.size() == 5)
                    {
                        message.Error = "Unknown error reported";
                        LOGDEBUG("Protocol Serializer error: " << message.Error);
                    }
                    else
                    {
                        message.Error = serializedData[5];
                        LOGDEBUG("Protocol Serializer message error: " << message.Error);
                    }
                    return message;
                }
                else
                {
                    message.Payload = data_t( serializedData.begin()+4, serializedData.end());
                }
            }
            return message;
        }

        Common::PluginProtocol::DataMessage ProtocolSerializer::createDefaultErrorMessage()
        {
            Common::PluginProtocol::DataMessage dataMessage;
            dataMessage.ProtocolVersion = m_supportedProtocolVersion;
            dataMessage.Command = Common::PluginProtocol::Commands::UNKNOWN;

            return dataMessage;
        }


    }

}