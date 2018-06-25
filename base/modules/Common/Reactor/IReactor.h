/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IREACTOR_H
#define EVEREST_BASE_IREACTOR_H

#include "ICallbackListener.h"
#include "IReadable.h"
#include "IShutdownListener.h"
namespace Common
{
    namespace Reactor
    {
        class IReactor
        {
        public:
            virtual ~IReactor() = default;
            virtual void registerListener( Common::ZeroMQWrapper::IReadable* , ICallbackListener * ) = 0;
            virtual void armShutdownListener(IShutdownListener *) = 0;
        };
    }
}

#endif //EVEREST_BASE_IREACTOR_H
