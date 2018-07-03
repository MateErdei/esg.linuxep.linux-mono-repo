//
// Created by pair on 02/07/18.
//

#ifndef EVEREST_BASE_SENSORDATAPUBLISHER_H
#define EVEREST_BASE_SENSORDATAPUBLISHER_H

#include "Common/PluginApi/ISensorDataPublisher.h"
#include "Common/ZeroMQWrapper/IContext.h"

namespace Common
{
    namespace PluginApiImpl
    {
        class SensorDataPublisher : public virtual Common::PluginApi::ISensorDataPublisher
        {
        public:
            SensorDataPublisher(const std::string& pluginName);
            void sendData(const std::string& sensorDataCategory, const std::string& sensorData) override;
        private:
            Common::ZeroMQWrapper::IContextPtr m_context;
            Common::ZeroMQWrapper::ISocketPublisherPtr m_socketPublisher;

        };
    }
}


#endif //EVEREST_BASE_SENSORDATAPUBLISHER_H
