/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "Common/PluginProtocol/IProtocolSerializer.h"
#include <memory>

namespace Common
{
    namespace PluginProtocol
    {
        class ProtocolSerializerFactory
        {
        public:
            std::unique_ptr<Common::PluginProtocol::IProtocolSerializer> createProtocolSerializer();
        };
    }
}



