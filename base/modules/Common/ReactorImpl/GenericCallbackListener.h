/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_GENERICCALLBACKLISTENER_H
#define EVEREST_BASE_GENERICCALLBACKLISTENER_H

#include "ICallbackListener.h"

namespace Common
{
    namespace Reactor
    {
        class GenericCallbackListener : ICallbackListener
        {
        public:
            void process(std::vector<std::string> ) override;
        };
    }
}

#endif //EVEREST_BASE_GENERICCALLBACKLISTENER_H
