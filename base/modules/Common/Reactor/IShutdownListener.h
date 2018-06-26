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

            /**
             * Used to inform the listener that a shutdown request was made.  Which is usually a linux signal sent to the process.
             */
            virtual void notifyShutdownRequested() = 0;
        };
    }
}

#endif //EVEREST_BASE_ISHUTDOWNLISTENER_H
