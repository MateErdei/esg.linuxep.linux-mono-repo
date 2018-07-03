//
// Created by pair on 28/06/18.
//

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
            virtual const data_t serialize(const DataMessage &data)const = 0;
            virtual const DataMessage deserialize(const data_t &data) = 0;
        };
    }

}

#endif //EVEREST_BASE_IPROTOCOLSERIALIZER_H