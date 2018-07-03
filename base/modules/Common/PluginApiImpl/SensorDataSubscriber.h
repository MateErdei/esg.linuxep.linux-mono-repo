//
// Created by pair on 02/07/18.
//

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
                                 std::shared_ptr<Common::PluginApi::ISensorDataCallback> sensorDataCallback);

            ~SensorDataSubscriber();
        private:
            void messageHandler(Common::ZeroMQWrapper::IReadable::data_t request) override ;

            std::shared_ptr<Common::ZeroMQWrapper::IContext> m_context;
            Common::ZeroMQWrapper::ISocketSubscriberPtr m_socketSubscriber;
            std::shared_ptr<Common::PluginApi::ISensorDataCallback> m_sensorDataCallback;
            std::unique_ptr<Common::Reactor::IReactor> m_reactor;
        };
    }
}

#endif //EVEREST_BASE_SENSORDATASUBSCRIBER_H
