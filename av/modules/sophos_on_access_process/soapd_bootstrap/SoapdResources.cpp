// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#include "SoapdResources.h"

#include "OnAccessRunner.h"

#include "common/PidLockFile.h"
#include "datatypes/SystemCallWrapper.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteClientSocketThread.h"

using namespace sophos_on_access_process;
using namespace sophos_on_access_process::soapd_bootstrap;
using namespace unixsocket::updateCompleteSocket;

std::unique_ptr<common::IPidLockFile> SoapdResources::getPidLockFile(const std::string& pidfile, bool changePidGroup)
{
    return std::make_unique<common::PidLockFile>(pidfile, changePidGroup);
}

datatypes::ISystemCallWrapperSharedPtr SoapdResources::getSystemCallWrapper()
{
    return std::make_shared<datatypes::SystemCallWrapper>();
}

service_impl::IOnAccessServicePtr SoapdResources::getOnAccessServiceImpl()
{
    return std::make_unique<service_impl::OnAccessServiceImpl>();
}

std::shared_ptr<common::AbstractThreadPluginInterface> SoapdResources::getUpdateClient(
    std::string socketPath,
    threat_scanner::IUpdateCompleteCallbackPtr callback)
{
    return std::make_shared<UpdateCompleteClientSocketThread>(socketPath, callback);
}
std::shared_ptr<common::AbstractThreadPluginInterface> SoapdResources::getProcessController(
    const std::string& socketPath,
    const std::string& userName,
    const std::string& groupName,
    mode_t mode,
    std::shared_ptr<unixsocket::IProcessControlMessageCallback> processControlCallback)
{
    return std::make_shared<unixsocket::ProcessControllerServerSocket>(
                     socketPath, userName, groupName, mode, processControlCallback);
}

std::shared_ptr<IOnAccessRunner> SoapdResources::getOnAccessRunner(
    datatypes::ISystemCallWrapperSharedPtr sysCallWrapper,
    onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr telemetryUtility)
{
    return std::make_shared<OnAccessRunner>(sysCallWrapper, telemetryUtility);
}
