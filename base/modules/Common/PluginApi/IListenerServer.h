/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IABSTRACTSERVER_H
#define EVEREST_BASE_IABSTRACTSERVER_H

#include "DataMessage.h"

#include <string>
#include <vector>

namespace Common
{
    namespace PluginApi
    {
        using data_t = std::vector<std::string>;

        class IListenerServer
        {
        public:
            virtual ~IListenerServer() = default;
            virtual DataMessage process(const DataMessage & request) const = 0;
            virtual void onShutdownRequested() = 0;
            virtual void start() = 0;
            virtual void stop() = 0;
        };
    }
}
#endif //EVEREST_BASE_IABSTRACTSERVER_H
