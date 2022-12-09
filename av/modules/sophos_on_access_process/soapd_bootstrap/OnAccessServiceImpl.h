// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "sophos_on_access_process/onaccessimpl/OnAccessTelemetryUtility.h"

#include "Common/PluginApiImpl/PluginCallBackHandler.h"
#include "Common/ZMQWrapperApi/IContext.h"

namespace sophos_on_access_process::service_impl
{
    class OnAccessServiceImpl
    {
    public:
        OnAccessServiceImpl();

        onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtilitySharedPtr getTelemetryUtility()
        {
            return m_TelemetryUtility;
        }

    private:
        Common::ZMQWrapperApi::IContextSharedPtr m_onAccessContext;
        std::unique_ptr<Common::PluginApiImpl::PluginCallBackHandler> m_pluginHandler;
        onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtilitySharedPtr m_TelemetryUtility;
    };
}