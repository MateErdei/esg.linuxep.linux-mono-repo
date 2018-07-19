/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/Reactor/IShutdownListener.h>
#include <functional>
#include "Common/Reactor/ICallbackListener.h"
namespace Common
{
    namespace ReactorImpl
    {
    class GenericShutdownListener : public virtual Reactor::IShutdownListener
        {
        public:
            explicit GenericShutdownListener( std::function<void()> callback);
            void notifyShutdownRequested() override;

        private:
            std::function<void()> m_callback;

        };

    }
}




