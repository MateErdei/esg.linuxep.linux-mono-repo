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
    namespace Reactor
    {
        class GenericCallbackListener : public virtual ICallbackListener
        {
        public:
            using CallbackFunction = std::function<void(std::vector<std::string>)>;
            explicit GenericCallbackListener(CallbackFunction callback);
            void process(std::vector<std::string> data ) override;
        private:
            CallbackFunction m_callback;
        };
    }
}

#endif //EVEREST_BASE_GENERICCALLBACKLISTENER_H
