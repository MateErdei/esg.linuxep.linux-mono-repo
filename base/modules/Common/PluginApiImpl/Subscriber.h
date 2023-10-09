/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISubscriber.h"


#include <Common/PluginApi/IEventVisitorCallback.h>
#include <Common/Reactor/IReactor.h>
#include <Common/ZMQWrapperApi/IContextPtr.h>
#include <Common/ZeroMQWrapper/ISocketSubscriberPtr.h>

namespace Common
{
    namespace PluginApiImpl
    {
        class SensorDataSubscriber : public virtual Common::PluginApi::ISubscriber,
                                     public Common::Reactor::ICallbackListener
        {
        public:
            SensorDataSubscriber(
                const std::string& sensorDataCategorySubscription,
                std::shared_ptr<Common::PluginApi::IEventVisitorCallback> sensorDataCallback,
                Common::ZeroMQWrapper::ISocketSubscriberPtr socketSubscriber);

            SensorDataSubscriber(
                const std::string& sensorDataCategorySubscription,
                std::shared_ptr<Common::PluginApi::IRawDataCallback> rawDataCallback,
                Common::ZeroMQWrapper::ISocketSubscriberPtr socketSubscriber);

            ~SensorDataSubscriber() noexcept override;
            void start() override;
            // cppcheck-suppress virtualCallInConstructor
            void stop() override;

        private:
            void messageHandler(Common::ZeroMQWrapper::IReadable::data_t request) override;

            Common::ZeroMQWrapper::ISocketSubscriberPtr m_socketSubscriber;
            std::unique_ptr<Common::Reactor::IReactor> m_reactor;
            std::shared_ptr<Common::PluginApi::IEventVisitorCallback> m_sensorDataCallback;
            std::shared_ptr<Common::PluginApi::IRawDataCallback> m_rawDataCallback;
            std::unique_ptr<Common::EventTypes::IEventConverter> m_converter;
        };
    } // namespace PluginApiImpl
} // namespace Common
