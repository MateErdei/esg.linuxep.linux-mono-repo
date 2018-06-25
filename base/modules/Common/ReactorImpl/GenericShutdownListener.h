/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_GENERICSHUTDOWNLISTENER_H
#define EVEREST_BASE_GENERICSHUTDOWNLISTENER_H

#include <Common/Reactor/IShutdownListener.h>
#include "ICallbackListener.h"
namespace Common
{
    namespace Reactor
    {
        class GenericShutdownListener : public IShutdownListener
        {
        public:
            void notifyShutdownRequested() override;
        };

    }
}



#endif //EVEREST_BASE_GENERICSHUTDOWNLISTENER_H
