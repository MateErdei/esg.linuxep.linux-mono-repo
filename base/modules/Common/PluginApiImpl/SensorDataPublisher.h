/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_SENSORDATAPUBLISHER_H
#define EVEREST_BASE_SENSORDATAPUBLISHER_H

#include "Common/PluginApi/ISensorDataPublisher.h"
#include "Common/ZeroMQWrapper/ISocketPublisherPtr.h"

namespace Common
{
    namespace PluginApiImpl
    {
        class SensorDataPublisher : public virtual Common::PluginApi::ISensorDataPublisher
        {
        public:
            SensorDataPublisher(Common::ZeroMQWrapper::ISocketPublisherPtr socketPublisher);
            void sendData(const std::string& sensorDataCategory, const std::string& sensorData) override;
        private:
            Common::ZeroMQWrapper::ISocketPublisherPtr m_socketPublisher;
        };
    }
}


#endif //EVEREST_BASE_SENSORDATAPUBLISHER_H
