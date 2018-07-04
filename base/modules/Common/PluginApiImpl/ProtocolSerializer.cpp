/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProtocolSerializer.h"
#include "ProtocolSerializerFactory.h"

namespace Common
{
    namespace PluginApiImpl
    {
        const data_t ProtocolSerializer::serialize(const Common::PluginApi::DataMessage &dataMessage)const
        {
            data_t data;
            data.push_back(dataMessage.ProtocolVersion);
            data.push_back(dataMessage.ApplicationId);
            data.push_back(PluginApi::SerializeCommand(dataMessage.Command));
            data.push_back(dataMessage.MessageId);

            if (dataMessage.Error.empty())
            {
                data.insert(data.begin()+data.size(),dataMessage.payload.begin(), dataMessage.payload.end());
            }
            else
            {
                data.push_back(ProtocolSerializerFactory::ProtocolErrorMark);
                data.push_back(dataMessage.Error);
            }

            return data;
        }

        const Common::PluginApi::DataMessage ProtocolSerializer::deserialize(const data_t &serializedData)
        {
            Common::PluginApi::DataMessage message;
            if ( serializedData.size() < 5 )
            {
                message.Error = "Bad formed message";
                return message;
            }

            message.ApplicationId = serializedData[1];
            message.Command = PluginApi::DeserializeCommand(serializedData[2]);
            message.MessageId = serializedData[3];

            if ( serializedData[4] == ProtocolSerializerFactory::ProtocolErrorMark)
            {
                if ( serializedData.size() == 5)
                {
                    message.Error = "Unknown error reported";
                }
                else
                {
                    message.Error = serializedData[5];
                }
                return message;
            }
            else
            {
                message.payload = data_t( serializedData.begin()+4, serializedData.end());
            }
            return message;
        }

    }

}