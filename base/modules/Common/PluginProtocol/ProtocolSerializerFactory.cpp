/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProtocolSerializerFactory.h"
#include "ProtocolSerializer.h"
#include "Logger.h"

namespace Common
{
    namespace PluginProtocol
    {
        std::unique_ptr<Common::PluginProtocol::IProtocolSerializer> ProtocolSerializerFactory::createProtocolSerializer()
        {
            return std::unique_ptr<Common::PluginProtocol::IProtocolSerializer>(new Common::PluginProtocol::ProtocolSerializer());
        }

    }
}