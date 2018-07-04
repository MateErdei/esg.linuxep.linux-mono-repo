//
// Created by pair on 02/07/18.
//

#include "SensorDataPublisher.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "SharedSocketContext.h"
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
