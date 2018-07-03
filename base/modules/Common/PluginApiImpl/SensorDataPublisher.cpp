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

        SensorDataPublisher::SensorDataPublisher(const std::string& pluginName)
        {
            m_context = sharedContext();
            m_socketPublisher = m_context->getPublisher();
            m_socketPublisher->connect(ApplicationConfiguration::applicationPathManager().getPublisherDataChannelAddress());
        }


        void SensorDataPublisher::sendData(const std::string &sensorDataCategory, const std::string &sensorData)
        {
            m_socketPublisher->write({sensorDataCategory, sensorData});
        }
    }

}
std::unique_ptr<Common::PluginApi::ISensorDataPublisher>
        Common::PluginApi::ISensorDataPublisher::newSensorDataPublisher(const std::string &pluginName)
{
    return std::unique_ptr<Common::PluginApi::ISensorDataPublisher> (new Common::PluginApiImpl::SensorDataPublisher(pluginName));
}
