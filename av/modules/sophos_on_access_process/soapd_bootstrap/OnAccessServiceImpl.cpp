// Copyright 2022-2023 Sophos Limited. All rights reserved.

// Class
#include "OnAccessServiceImpl.h"

// Package
#include "OnAccessServiceCallback.h"

#include "sophos_on_access_process/OnAccessTelemetryUtility/OnAccessTelemetryUtility.h"
#include "sophos_on_access_process/OnAccessTelemetryFields/OnAccessTelemetryFields.h"

// Base
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"

namespace sophos_on_access_process::service_impl
{
    using namespace service_callback;
    using namespace onaccessimpl::onaccesstelemetry;

    OnAccessServiceImpl::OnAccessServiceImpl() : m_TelemetryUtility(std::make_shared<OnAccessTelemetryUtility>())
    {
        Common::Telemetry::TelemetryHelper::getInstance().restore(ON_ACCESS_TELEMETRY_SOCKET);

        m_onAccessContext = Common::ZMQWrapperApi::createContext();
        auto replier = m_onAccessContext->getReplier();
        Common::PluginApiImpl::PluginResourceManagement::setupReplier(*replier, ON_ACCESS_TELEMETRY_SOCKET, 5000, 5000);

        auto pluginCallback = std::make_shared<OnAccessServiceCallback>(m_TelemetryUtility);

        m_pluginHandler = std::make_unique<Common::PluginApiImpl::PluginCallBackHandler>
            (ON_ACCESS_TELEMETRY_SOCKET,
             std::move(replier),
             pluginCallback,
             Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY::DONOTARM);
        m_pluginHandler->start();
    }
}