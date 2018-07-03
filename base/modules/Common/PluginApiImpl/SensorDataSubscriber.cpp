//
// Created by pair on 02/07/18.
//

#include "Common/ZeroMQWrapper/IContext.h"
#include "Common/ZeroMQWrapper/ISocketSubscriber.h"
#include "Common/PluginApi/ISensorDataSubscriber.h"
#include "Common/PluginApi/ISensorDataCallback.h"
#include "SensorDataSubscriber.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

namespace Common
{
    namespace PluginApiImpl
    {
        SensorDataSubscriber::SensorDataSubscriber(const std::string &sensorDataCategorySubscription,
                                                   std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback)
        : m_context(Common::ZeroMQWrapper::createContext())
        {
            m_socketSubscriber = m_context->getSubscriber();
            m_socketSubscriber->connect(Common::ApplicationConfiguration::applicationPathManager().getSubscriberDataChannelAddress());
            m_socketSubscriber->subscribeTo(sensorDataCategorySubscription);

            m_sensorDataCallback = sensorDataCallback;
            m_reactor = Common::Reactor::createReactor();
            m_reactor->addListener(m_socketSubscriber.get(), this);
            m_reactor->start();

        }

        void SensorDataSubscriber::messageHandler(Common::ZeroMQWrapper::IReadable::data_t request)
        {
            const std::string & key = request.at(0);
            const std::string & data = request.at(1);
            m_sensorDataCallback->receiveData(key, data);
        }

        SensorDataSubscriber::~SensorDataSubscriber()
        {
            if ( m_reactor)
            {
                m_reactor->stop();
            }
        }
    }

}


std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> Common::PluginApi::ISensorDataSubscriber::newSensorDataSubscriber(const std::string & sensorDataCategorySubscription,
                                                                      std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback)
{
    return std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> (new Common::PluginApiImpl::SensorDataSubscriber(sensorDataCategorySubscription, sensorDataCallback));
}