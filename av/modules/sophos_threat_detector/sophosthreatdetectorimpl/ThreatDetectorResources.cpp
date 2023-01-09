// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ThreatDetectorResources.h"

#include "common/signals/SigTermMonitor.h"
#include "common/PidLockFile.h"
#include "datatypes/SystemCallWrapperFactory.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatReporter.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ShutdownTimer.h"
#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteServerSocket.h"

using namespace sspl::sophosthreatdetectorimpl;


datatypes::ISystemCallWrapperSharedPtr ThreatDetectorResources::createSystemCallWrapper()
{
    auto sysCallFact = datatypes::SystemCallWrapperFactory();
    return sysCallFact.createSystemCallWrapper();
}

common::signals::ISignalHandlerSharedPtr ThreatDetectorResources::createSignalHandler(bool restartSyscalls)
{
    return std::make_shared<common::signals::SigTermMonitor>(restartSyscalls);
}

common::IPidLockFileSharedPtr ThreatDetectorResources::createPidLockFile(const std::string& _path)
{
    return std::make_shared<common::PidLockFile>(_path);
}

threat_scanner::IThreatReporterSharedPtr ThreatDetectorResources::createThreatReporter(const sophos_filesystem::path _socketPath)
{
    return std::make_shared<ThreatReporter>(_socketPath);
}

threat_scanner::IScanNotificationSharedPtr ThreatDetectorResources::createShutdownTimer(const sophos_filesystem::path _configPath)
{
    return std::make_shared<ShutdownTimer>(_configPath);
}

threat_scanner::IThreatScannerFactorySharedPtr ThreatDetectorResources::createSusiScannerFactory(
    threat_scanner::IThreatReporterSharedPtr _reporter,
    threat_scanner::IScanNotificationSharedPtr _shutdownTimer,
    threat_scanner::IUpdateCompleteCallbackPtr _updateCompleteCallback)
{
    return std::make_shared<threat_scanner::SusiScannerFactory>(_reporter, _shutdownTimer, _updateCompleteCallback);
}

unixsocket::updateCompleteSocket::UpdateCompleteServerSocketPtr ThreatDetectorResources::createUpdateCompleteNotifier
    (const sophos_filesystem::path _serverPath, mode_t _mode)
{
    return std::make_shared<unixsocket::updateCompleteSocket::UpdateCompleteServerSocket>(_serverPath, _mode);
}

unixsocket::ScanningServerSocketPtr ThreatDetectorResources::createScanningServerSocket(
    const std::string& path,
    mode_t mode,
    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
    )
{
    return std::make_shared<unixsocket::ScanningServerSocket>(path, mode, scannerFactory);
}

unixsocket::ProcessControllerServerSocketPtr ThreatDetectorResources::createProcessControllerServerSocket(
    const std::string& path,
    mode_t mode,
    std::shared_ptr<unixsocket::IProcessControlMessageCallback> processControlCallbacks
    )
{
    return std::make_shared<unixsocket::ProcessControllerServerSocket>(path, mode, processControlCallbacks);
}