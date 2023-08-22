// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Reactor/ICallbackListener.h"

#include "Common/Reactor/IShutdownListener.h"

#include <functional>
namespace Common::ReactorImpl
{
    class GenericShutdownListener : public virtual Reactor::IShutdownListener
    {
    public:
        explicit GenericShutdownListener(std::function<void()> callback);
        void notifyShutdownRequested() override;

    private:
        std::function<void()> m_callback;
    };
} // namespace Common::ReactorImpl
