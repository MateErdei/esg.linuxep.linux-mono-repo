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
            static const char* ProtocolErrorMark;// = "Error";
            static const char* ProtocolVersion;// = "v1";
            std::unique_ptr<Common::PluginProtocol::IProtocolSerializer> createProtocolSerializer(
                    const std::string &protocolVersion);
        };
    }
}



