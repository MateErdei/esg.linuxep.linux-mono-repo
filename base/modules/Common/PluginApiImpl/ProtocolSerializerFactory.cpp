//
// Created by pair on 28/06/18.
//

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

            return nullptr;
        }

    }
}