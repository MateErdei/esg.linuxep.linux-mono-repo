// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#include "SoapdResources.h"

#include "datatypes/SystemCallWrapper.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteClientSocketThread.h"

using namespace sophos_on_access_process;
using namespace sophos_on_access_process::soapd_bootstrap;
using namespace unixsocket::updateCompleteSocket;

datatypes::ISystemCallWrapperSharedPtr SoapdResources::getSystemCallWrapper()
{
    return std::make_shared<datatypes::SystemCallWrapper>();
}

service_impl::IOnAccessServicePtr SoapdResources::getOnAccessServiceImpl()
{
    return std::make_unique<service_impl::OnAccessServiceImpl>();
}

std::shared_ptr<common::AbstractThreadPluginInterface> SoapdResources::getUpdateClient(
    std::string socket_path,
    threat_scanner::IUpdateCompleteCallbackPtr callback)
{
    return std::make_shared<UpdateCompleteClientSocketThread>(socket_path, callback);
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
