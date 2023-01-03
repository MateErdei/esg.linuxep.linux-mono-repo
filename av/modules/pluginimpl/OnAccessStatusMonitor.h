// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "PluginCallback.h"

#include "common/AbstractThreadPluginInterface.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/AutoFd.h"


namespace Plugin
{
    class OnAccessStatusMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        explicit OnAccessStatusMonitor(PluginCallbackSharedPtr callback);
        OnAccessStatusMonitor(const OnAccessStatusMonitor&) = delete;
        OnAccessStatusMonitor(OnAccessStatusMonitor&&) = delete;

        void run() override;
    private:
        void statusFileChanged();
        PluginCallbackSharedPtr m_callback;
    };
}
