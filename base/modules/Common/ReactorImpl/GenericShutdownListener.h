/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/Common/Reactor/ICallbackListener.h"

#include "modules/Common/Reactor/IShutdownListener.h"

#include <functional>
namespace Common
{
    namespace ReactorImpl
    {
        class GenericShutdownListener : public virtual Reactor::IShutdownListener
        {
        public:
            explicit GenericShutdownListener(std::function<void()> callback);
            void notifyShutdownRequested() override;

        private:
            std::function<void()> m_callback;
        };

    } // namespace ReactorImpl
} // namespace Common
