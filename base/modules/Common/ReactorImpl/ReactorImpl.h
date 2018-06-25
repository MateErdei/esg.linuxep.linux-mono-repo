/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_REACTORIMPL_H
#define EVEREST_BASE_REACTORIMPL_H

#include "IReactor.h"
#include "ICallbackListener.h"

namespace Common
{
    namespace Reactor
    {
        class ReactorImpl : public IReactor
        {
        public:
             void registerListener( Common::ZeroMQWrapper::IReadable* , ICallbackListener * ) override;
             void armShutdownListener(IShutdownListener *) override ;
        };
    }
}


#endif //EVEREST_BASE_REACTORIMPL_H
