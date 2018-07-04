/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IPROTOCOLSERIALIZATIONFACTORY_H
#define EVEREST_BASE_IPROTOCOLSERIALIZATIONFACTORY_H

#include <string>
#include <memory>
#include "IProtocolSerializer.h"

namespace Common
{
    namespace PluginApi
    {
        class IProtocolSerializerFactory
        {
        public:
            virtual  ~IProtocolSerializerFactory() = default;
            virtual std::unique_ptr<Common::PluginApi::IProtocolSerializer>  createProtocolSerializer(
                    const std::string &protocolVersion) = 0;
        };
    }

}


#endif //EVEREST_BASE_IPROTOCOLSERIALIZATIONFACTORY_H
