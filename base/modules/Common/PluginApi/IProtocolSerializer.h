/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IPROTOCOLSERIALIZER_H
#define EVEREST_BASE_IPROTOCOLSERIALIZER_H

#include "DataMessage.h"

#include <string>
#include <vector>

namespace Common
{
    namespace PluginApi
    {
        using data_t  = std::vector<std::string>;

        class IProtocolSerializer
        {

        public:
            virtual ~IProtocolSerializer() = default;
            virtual const data_t serialize(const DataMessage &data)const = 0;
            virtual const DataMessage deserialize(const data_t &data) = 0;
        };
    }

}

#endif //EVEREST_BASE_IPROTOCOLSERIALIZER_H