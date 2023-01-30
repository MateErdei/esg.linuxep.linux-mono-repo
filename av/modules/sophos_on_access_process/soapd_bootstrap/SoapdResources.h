// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include "ISoapdResources.h"
#include "OnAccessServiceImpl.h"

#include "datatypes/ISystemCallWrapper.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdResources : public ISoapdResources
    {
    public:
        ~SoapdResources() override = default;

        std::unique_ptr<common::IPidLockFile> getPidLockFile(const std::string& pidfile, bool changePidGroup) override;

        datatypes::ISystemCallWrapperSharedPtr getSystemCallWrapper() override;
        service_impl::IOnAccessServicePtr getOnAccessServiceImpl() override;

        std::shared_ptr<common::AbstractThreadPluginInterface> getUpdateClient(
            std::string socketPath,
            threat_scanner::IUpdateCompleteCallbackPtr callback
            ) override;

        std::shared_ptr<common::AbstractThreadPluginInterface> getProcessController(
            const std::string& socketPath,
            const std::string& userName,
            const std::string& groupName,
            mode_t mode,
            std::shared_ptr<unixsocket::IProcessControlMessageCallback> processControlCallback
            ) override;

        std::shared_ptr<IOnAccessRunner> getOnAccessRunner(
            datatypes::ISystemCallWrapperSharedPtr sysCallWrapper,
            onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr telemetryUtility
            ) override;
    };
} // namespace sophos_on_access_process::soapd_bootstrap
