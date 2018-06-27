/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_GENERICCALLBACKLISTENER_H
#define EVEREST_BASE_GENERICCALLBACKLISTENER_H

#include "ICallbackListener.h"
#include <string>
#include <vector>
#include <functional>
namespace Common
{
    namespace ReactorImpl
    {
        class GenericCallbackListener : public virtual Reactor::ICallbackListener
        {
        public:
            using CallbackFunction = std::function<void(Common::ZeroMQWrapper::IReadable::data_t)>;
            explicit GenericCallbackListener(CallbackFunction callback);
            void process(Common::ZeroMQWrapper::IReadable::data_t data ) override;
        private:
            CallbackFunction m_callback;
        };
    }
}

#endif //EVEREST_BASE_GENERICCALLBACKLISTENER_H
