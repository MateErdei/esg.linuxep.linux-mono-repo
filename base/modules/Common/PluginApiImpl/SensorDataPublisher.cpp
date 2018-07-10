/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SensorDataPublisher.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
namespace Common
{
    namespace PluginApiImpl
    {

        void SensorDataPublisher::sendData(const std::string &sensorDataCategory, const std::string &sensorData)
        {
            m_socketPublisher->write({sensorDataCategory, sensorData});
        }

        SensorDataPublisher::SensorDataPublisher(Common::ZeroMQWrapper::ISocketPublisherPtr socketPublisher)
        : m_socketPublisher(std::move(socketPublisher))
        {

        }
    }

}
