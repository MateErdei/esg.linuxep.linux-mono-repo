/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_GENERICSHUTDOWNLISTENER_H
#define EVEREST_BASE_GENERICSHUTDOWNLISTENER_H

#include <Common/Reactor/IShutdownListener.h>
#include <functional>
#include "ICallbackListener.h"
namespace Common
{
    namespace Reactor
    {
        class GenericShutdownListener : public virtual IShutdownListener
        {
        public:
            explicit GenericShutdownListener( std::function<void()> callback);
            void notifyShutdownRequested() override;

        private:
            std::function<void()> m_callback;

        };

    }
}



#endif //EVEREST_BASE_GENERICSHUTDOWNLISTENER_H
