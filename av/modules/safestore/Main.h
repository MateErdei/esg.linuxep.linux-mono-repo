// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "Common/PluginApi/IPluginCallbackApi.h"
#include "Common/PluginApiImpl/PluginCallBackHandler.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZMQWrapperApi/IContextSharedPtr.h"

namespace safestore
{
    class Main
    {
    public:
        ~Main();
        static int run();
        static std::string SafeStoreServiceLineName() { return "safestore"; }

    private:
        void innerRun();
        Common::ZMQWrapperApi::IContextSharedPtr m_context = Common::ZMQWrapperApi::createContext();
        std::unique_ptr<Common::PluginApiImpl::PluginCallBackHandler> m_pluginHandler;
    };
} // namespace safestore
