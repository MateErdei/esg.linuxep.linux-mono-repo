/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProtocolSerializerFactory.h"

#include "ProtocolSerializer.h"

#include <Common/PluginApi/Logger.h>

namespace Common
{
    namespace PluginProtocol
    {
        const char*  ProtocolSerializerFactory::ProtocolErrorMark = "Error";
        const char*  ProtocolSerializerFactory::ProtocolVersion = "v1";

        std::unique_ptr<Common::PluginProtocol::IProtocolSerializer> ProtocolSerializerFactory::createProtocolSerializer(
                const std::string &protocolVersion)
        {

            // using protocol verson 1.
            if(protocolVersion == ProtocolSerializerFactory::ProtocolVersion)
            {
                return std::unique_ptr<Common::PluginProtocol::IProtocolSerializer>(new Common::PluginProtocol::ProtocolSerializer());
            }

            // default ProtocolSerializer so we can correctly respond with an error in a message.
            return std::unique_ptr<Common::PluginProtocol::IProtocolSerializer>(new Common::PluginProtocol::ProtocolSerializer());
        }

    }
}