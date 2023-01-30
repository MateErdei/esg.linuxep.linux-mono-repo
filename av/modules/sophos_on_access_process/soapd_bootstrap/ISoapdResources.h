// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include "IOnAccessRunner.h"
#include "IOnAccessService.h"

#include "common/AbstractThreadPluginInterface.h"
#include "common/IPidLockFile.h"
#include "datatypes/ISystemCallWrapper.h"
#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"
#include "unixsocket/processControllerSocket/IProcessControlMessageCallback.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class ISoapdResources
    {
    public:
        virtual ~ISoapdResources() = default;

        virtual std::unique_ptr<common::IPidLockFile> getPidLockFile(
            const std::string& pidfile,
            bool changePidGroup) = 0;

        virtual datatypes::ISystemCallWrapperSharedPtr getSystemCallWrapper() = 0;
        virtual service_impl::IOnAccessServicePtr getOnAccessServiceImpl() = 0;

        virtual std::shared_ptr<common::AbstractThreadPluginInterface> getUpdateClient(
            std::string socketPath,
            threat_scanner::IUpdateCompleteCallbackPtr callback) = 0;

        virtual std::shared_ptr<common::AbstractThreadPluginInterface> getProcessController(
            const std::string& socketPath,
            const std::string& userName,
            const std::string& groupName,
            mode_t mode,
            std::shared_ptr<unixsocket::IProcessControlMessageCallback> processControlCallback) = 0;

        virtual std::shared_ptr<IOnAccessRunner> getOnAccessRunner(
            datatypes::ISystemCallWrapperSharedPtr sysCallWrapper,
            onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr telemetryUtility
            ) = 0;
    };
} // namespace sophos_on_access_process::soapd_bootstrap
