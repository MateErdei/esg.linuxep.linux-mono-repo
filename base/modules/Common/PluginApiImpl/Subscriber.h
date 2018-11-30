/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/ZeroMQWrapper/IContextPtr.h>
#include <Common/ZeroMQWrapper/ISocketSubscriberPtr.h>
#include <Common/Reactor/IReactor.h>
#include "ISubscriber.h"
namespace Common
{
    namespace PluginApiImpl
    {
        class SensorDataSubscriber : public virtual Common::PluginApi::ISubscriber, public Common::Reactor::ICallbackListener
        {
        public:
            SensorDataSubscriber(const std::string & sensorDataCategorySubscription,
                                 std::shared_ptr<Common::PluginApi::IRawDataCallback> sensorDataCallback,
                                 Common::ZeroMQWrapper::ISocketSubscriberPtr socketSubscriber);

            ~SensorDataSubscriber() override;
            void start() override ;
            void stop() override ;
        private:
            void messageHandler(Common::ZeroMQWrapper::IReadable::data_t request) override ;

            Common::ZeroMQWrapper::ISocketSubscriberPtr m_socketSubscriber;
            std::unique_ptr<Common::Reactor::IReactor> m_reactor;
            std::shared_ptr<Common::PluginApi::IRawDataCallback> m_sensorDataCallback;
        };
    }
}


