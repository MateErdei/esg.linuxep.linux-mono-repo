/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_SENSORDATASUBSCRIBER_H
#define EVEREST_BASE_SENSORDATASUBSCRIBER_H

#include <Common/ZeroMQWrapper/IContextPtr.h>
#include <Common/ZeroMQWrapper/ISocketSubscriberPtr.h>
#include <Common/Reactor/IReactor.h>
#include "ISensorDataSubscriber.h"
namespace Common
{
    namespace PluginApiImpl
    {
        class SensorDataSubscriber : public virtual Common::PluginApi::ISensorDataSubscriber, public Common::Reactor::ICallbackListener
        {
        public:
            SensorDataSubscriber(const std::string & sensorDataCategorySubscription,
                                 std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback,
                                 Common::ZeroMQWrapper::ISocketSubscriberPtr socketSubscriber);

            ~SensorDataSubscriber();
            void start() override ;
            void stop() override ;
        private:
            void messageHandler(Common::ZeroMQWrapper::IReadable::data_t request) override ;

            Common::ZeroMQWrapper::ISocketSubscriberPtr m_socketSubscriber;
            std::unique_ptr<Common::Reactor::IReactor> m_reactor;
            std::shared_ptr<Common::PluginApi::ISensorDataCallback> m_sensorDataCallback;
        };
    }
}

#endif //EVEREST_BASE_SENSORDATASUBSCRIBER_H
