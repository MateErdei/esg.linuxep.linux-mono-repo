/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_ISHUTDOWNLISTENER_H
#define EVEREST_BASE_ISHUTDOWNLISTENER_H

namespace Common
{
    namespace Reactor
    {
        class IShutdownListener
        {
        public:
            virtual ~IShutdownListener() = default;
            virtual void notifyShutdownRequested() = 0;
        };
    }
}

#endif //EVEREST_BASE_ISHUTDOWNLISTENER_H
