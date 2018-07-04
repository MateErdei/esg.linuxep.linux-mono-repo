//
// Created by pair on 29/06/18.
//

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
            static std::unique_ptr<ISensorDataPublisher> newSensorDataPublisher(const std::string& pluginName);
            virtual void sendData(const std::string& sensorDataCategory, const std::string& sensorData) = 0;
        };
    }
}

#endif //EVEREST_BASE_ISENSORDATAPUBLISHER_H
