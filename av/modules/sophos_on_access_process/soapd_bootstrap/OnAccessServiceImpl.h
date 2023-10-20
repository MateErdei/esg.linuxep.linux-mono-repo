// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IOnAccessService.h"

#include "sophos_on_access_process/IOnAccessTelemetryUtility/IOnAccessTelemetryUtility.h"

#include "Common/PluginApiImpl/PluginCallBackHandler.h"
#include "Common/ZMQWrapperApi/IContext.h"

namespace sophos_on_access_process::service_impl
{
    class OnAccessServiceImpl : public IOnAccessService
    {
    public:
        OnAccessServiceImpl();

        onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr getTelemetryUtility() override
        {
            return m_TelemetryUtility;
        }

    private:
        Common::ZMQWrapperApi::IContextSharedPtr m_onAccessContext;
        std::unique_ptr<Common::PluginApiImpl::PluginCallBackHandler> m_pluginHandler;
        onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr m_TelemetryUtility;
    };
}