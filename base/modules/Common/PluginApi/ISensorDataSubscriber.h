/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_ISENSORDATASUBSCRIBER_H
#define EVEREST_BASE_ISENSORDATASUBSCRIBER_H
#include <memory>
#include "ISensorDataCallback.h"
namespace Common
{
    namespace PluginApi
    {
        class ISensorDataSubscriber
        {
        public:
            virtual  ~ISensorDataSubscriber() = default;
            virtual void start() = 0;
            virtual void stop() = 0;
        };
    }
}

#endif //EVEREST_BASE_ISENSORDATASUBSCRIBER_H
