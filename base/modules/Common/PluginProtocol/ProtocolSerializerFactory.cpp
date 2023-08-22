/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProtocolSerializerFactory.h"

#include "Logger.h"
#include "ProtocolSerializer.h"

namespace Common::PluginProtocol
{
    std::unique_ptr<Common::PluginProtocol::IProtocolSerializer> ProtocolSerializerFactory::
        createProtocolSerializer()
    {
        return std::unique_ptr<Common::PluginProtocol::IProtocolSerializer>(
            new Common::PluginProtocol::ProtocolSerializer());
    }
} // namespace Common::PluginProtocol