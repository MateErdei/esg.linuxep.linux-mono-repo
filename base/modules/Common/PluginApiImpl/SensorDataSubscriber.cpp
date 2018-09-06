/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SensorDataSubscriber.h"
#include "Logger.h"
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/PluginApi/ISensorDataSubscriber.h>



namespace Common
{
    namespace PluginApiImpl
    {
        SensorDataSubscriber::SensorDataSubscriber(const std::string &sensorDataCategorySubscription,
                                                   std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback,
                                                   Common::ZeroMQWrapper::ISocketSubscriberPtr socketSubscriber):
        m_socketSubscriber(std::move(socketSubscriber)), m_reactor(Common::Reactor::createReactor()), m_sensorDataCallback(sensorDataCallback)
        {
            m_socketSubscriber->subscribeTo(sensorDataCategorySubscription);
            m_reactor->addListener(m_socketSubscriber.get(), this);
        }



        void SensorDataSubscriber::messageHandler(Common::ZeroMQWrapper::IReadable::data_t request)
        {
            const std::string & key = request.at(0);
            const std::string & data = request.at(1);
            m_sensorDataCallback->receiveData(key, data);
        }

        SensorDataSubscriber::~SensorDataSubscriber()
        {
            stop();
        }

        void SensorDataSubscriber::start()
        {
            LOGSUPPORT("Starting SensorDataSubscriber reactor");
            m_reactor->start();
        }

        void SensorDataSubscriber::stop()
        {
            LOGSUPPORT("Stopping SensorDataSubscriber reactor");
            m_reactor->stop();
        }

    }

}


//std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> Common::PluginApi::ISensorDataSubscriber::newSensorDataSubscriber(const std::string & sensorDataCategorySubscription,
//                                                                      std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback)
//{
//    return std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> (new Common::PluginApiImpl::SensorDataSubscriber(sensorDataCategorySubscription, sensorDataCallback));
//}