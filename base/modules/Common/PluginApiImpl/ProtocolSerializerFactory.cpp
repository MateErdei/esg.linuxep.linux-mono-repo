/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProtocolSerializerFactory.h"

#include "PluginApiImpl/ProtocolSerializer.h"

namespace Common
{
    namespace PluginApiImpl
    {
        const char*  ProtocolSerializerFactory::ProtocolErrorMark = "Error";
        const char*  ProtocolSerializerFactory::ProtocolVersion = "v1";

        std::unique_ptr<Common::PluginApi::IProtocolSerializer> ProtocolSerializerFactory::createProtocolSerializer(
                const std::string &protocolVersion)
        {
            // using protocol verson 1.
            if(protocolVersion == ProtocolSerializerFactory::ProtocolVersion)
            {
                return std::unique_ptr<Common::PluginApi::IProtocolSerializer>(new Common::PluginApiImpl::ProtocolSerializer());
            }

            // default ProtocolSerializer so we can correctly respond with an error in a message.
            return std::unique_ptr<Common::PluginApi::IProtocolSerializer>(new Common::PluginApiImpl::ProtocolSerializer());
        }

    }
}