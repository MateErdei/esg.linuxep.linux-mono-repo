/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Reactor/ICallbackListener.h"

#include <functional>
#include <string>
#include <vector>
namespace Common
{
    namespace ReactorImpl
    {
        class GenericCallbackListener : public virtual Reactor::ICallbackListener
        {
        public:
            using CallbackFunction = std::function<void(Common::ZeroMQWrapper::IReadable::data_t)>;
            explicit GenericCallbackListener(CallbackFunction callback);
            void messageHandler(Common::ZeroMQWrapper::IReadable::data_t data) override;

        private:
            CallbackFunction m_callback;
        };
    } // namespace ReactorImpl
} // namespace Common
