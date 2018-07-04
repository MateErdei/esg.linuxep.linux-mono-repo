/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PROTOCOLSERIALIZATIONFACTORY_H
#define EVEREST_BASE_PROTOCOLSERIALIZATIONFACTORY_H

#include <Common/PluginApi/IProtocolSerializer.h>
#include "Common/PluginApi/IProtocolSerializerFactory.h"


namespace Common
{
    namespace PluginApiImpl
    {
        class ProtocolSerializerFactory : public Common::PluginApi::IProtocolSerializerFactory
        {
        public:
            static const char* ProtocolErrorMark;// = "Error";
            static const char* ProtocolVersion;// = "v1";
            std::unique_ptr<Common::PluginApi::IProtocolSerializer> createProtocolSerializer(
                    const std::string &protocolVersion) override;
        };
    }
}


#endif //EVEREST_BASE_PROTOCOLSERIALIZATIONFACTORY_H
