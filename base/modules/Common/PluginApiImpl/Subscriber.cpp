/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Subscriber.h"
#include "Logger.h"

#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/PluginApi/ISubscriber.h>
#include <Common/EventTypesImpl/EventConverter.h>
#include <Common/PluginApi/AbstractEventVisitor.h>

namespace Common
{
    namespace PluginApiImpl
    {
        SensorDataSubscriber::SensorDataSubscriber(const std::string &sensorDataCategorySubscription,
                                                   std::shared_ptr<Common::PluginApi::IEventVisitorCallback> sensorDataCallback,
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
            if(key == "Detector.Credentials")
            {
                Common::EventTypes::EventConverter converter;
                Common::EventTypes::CredentialEvent event = *converter.createEventFromString<Common::EventTypes::CredentialEvent>(data).get();
                m_sensorDataCallback->processEvent(event);
            }
            else if(key == "Detector.PortScanning")
            {
                Common::EventTypes::EventConverter converter;
                Common::EventTypes::PortScanningEvent event = *converter.createEventFromString<Common::EventTypes::PortScanningEvent>(data).get();
                m_sensorDataCallback->processEvent(event);
            }
            else
            {
                LOGERROR("Unknown event received, received event id = '" << key << "'");
            }
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
