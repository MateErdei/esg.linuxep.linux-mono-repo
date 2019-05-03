/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Subscriber.h"

#include "Logger.h"

#include <Common/EventTypes/EventStrings.h>
#include <Common/EventTypesImpl/EventConverter.h>
#include <Common/Exceptions/Print.h>
#include <Common/PluginApi/AbstractEventVisitor.h>
#include <Common/PluginApi/ISubscriber.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>

namespace Common
{
    namespace PluginApiImpl
    {
        SensorDataSubscriber::SensorDataSubscriber(
            const std::string& sensorDataCategorySubscription,
            std::shared_ptr<Common::PluginApi::IEventVisitorCallback> sensorDataCallback,
            Common::ZeroMQWrapper::ISocketSubscriberPtr socketSubscriber) :
            m_socketSubscriber(std::move(socketSubscriber)),
            m_reactor(Common::Reactor::createReactor()),
            m_sensorDataCallback(std::move(sensorDataCallback)),
            m_converter(Common::EventTypes::constructEventConverter())
        {
            m_socketSubscriber->subscribeTo(sensorDataCategorySubscription);
            m_reactor->addListener(m_socketSubscriber.get(), this);
        }

        SensorDataSubscriber::SensorDataSubscriber(
            const std::string& sensorDataCategorySubscription,
            std::shared_ptr<Common::PluginApi::IRawDataCallback> rawDataCallback,
            Common::ZeroMQWrapper::ISocketSubscriberPtr socketSubscriber) :
            m_socketSubscriber(std::move(socketSubscriber)),
            m_reactor(Common::Reactor::createReactor()),
            m_rawDataCallback(std::move(rawDataCallback))
        {
            m_socketSubscriber->subscribeTo(sensorDataCategorySubscription);
            m_reactor->addListener(m_socketSubscriber.get(), this);
        }

        void SensorDataSubscriber::messageHandler(Common::ZeroMQWrapper::IReadable::data_t request)
        {
            const std::string& key = request.at(0);
            const std::string& data = request.at(1);

            if (m_sensorDataCallback)
            {
                if (key == Common::EventTypes::CredentialEventName)
                {
                    Common::EventTypes::CredentialEvent event = m_converter->stringToCredentialEvent(data);
                    m_sensorDataCallback->processEvent(event);
                }
                else if (key == Common::EventTypes::PortScanningEventName)
                {
                    Common::EventTypes::PortScanningEvent event = m_converter->stringToPortScanningEvent(data);
                    m_sensorDataCallback->processEvent(event);
                }
                else if (key == Common::EventTypes::ProcessEventName)
                {
                    Common::EventTypes::ProcessEvent event = m_converter->stringToProcessEvent(data);
                    m_sensorDataCallback->processEvent(event);
                }
                else
                {
                    m_sensorDataCallback->receiveData(key, data);
                }
            }
            else
            {
                assert(m_rawDataCallback);
                m_rawDataCallback->receiveData(key, data);
            }
        }

        SensorDataSubscriber::~SensorDataSubscriber() noexcept
        {
            try
            {
                stop();
            }
            catch (const std::exception& ex)
            {
                PRINT("Exception caught while attempting to stop SensorDataSubscriber in destructor: "<<ex.what());
            }
            catch(...)
            {
                PRINT("Non std::exception caught while attempting to stop SensorDataSubscriber in destructor");
            }
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
            m_reactor->join();
        }

    } // namespace PluginApiImpl

} // namespace Common
