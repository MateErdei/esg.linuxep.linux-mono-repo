/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_ISENSORDATAPUBLISHER_H
#define EVEREST_BASE_ISENSORDATAPUBLISHER_H

#include <memory>
namespace Common
{
    namespace PluginApi
    {
        class ISensorDataPublisher
        {
        public:
            virtual ~ISensorDataPublisher() = default;
            virtual void sendData(const std::string& sensorDataCategory, const std::string& sensorData) = 0;
        };
    }
}

#endif //EVEREST_BASE_ISENSORDATAPUBLISHER_H
