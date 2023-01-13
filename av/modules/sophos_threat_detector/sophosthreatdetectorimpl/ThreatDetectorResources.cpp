// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#include "Reloader.h"
#include "SafeStoreRescanWorker.h"
#include "ThreatDetectorResources.h"
#include "ThreatDetectorControlCallback.h"

#include "common/PidLockFile.h"
#include "common/signals/SigTermMonitor.h"
#include "common/signals/SigUSR1Monitor.h"
#include "datatypes/SystemCallWrapperFactory.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ShutdownTimer.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatReporter.h"
#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteServerSocket.h"

using namespace sspl::sophosthreatdetectorimpl;

datatypes::ISystemCallWrapperSharedPtr ThreatDetectorResources::createSystemCallWrapper()
{
    auto sysCallFact = datatypes::SystemCallWrapperFactory();
    return sysCallFact.createSystemCallWrapper();
}

common::signals::ISignalHandlerSharedPtr ThreatDetectorResources::createSigTermHandler(bool _restartSyscalls)
{
    return std::make_shared<common::signals::SigTermMonitor>(_restartSyscalls);
}

common::signals::ISignalHandlerSharedPtr ThreatDetectorResources::createUsr1Monitor(common::signals::IReloadablePtr _reloadable)
{
    return std::make_shared<common::signals::SigUSR1Monitor>(_reloadable);
}

std::shared_ptr<common::signals::IReloadable> ThreatDetectorResources::createReloader(threat_scanner::IThreatScannerFactorySharedPtr _scannerFactory)
{
    return std::make_shared<Reloader>(_scannerFactory);
}

common::IPidLockFileSharedPtr ThreatDetectorResources::createPidLockFile(const std::string& _path)
{
    return std::make_shared<common::PidLockFile>(_path);
}

ISafeStoreRescanWorkerPtr ThreatDetectorResources::createSafeStoreRescanWorker(const sophos_filesystem::path& _safeStoreRescanSocket)
{
    return std::make_shared<SafeStoreRescanWorker>(_safeStoreRescanSocket);
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
    const std::string& _path,
    mode_t _mode,
    threat_scanner::IThreatScannerFactorySharedPtr _scannerFactory
    )
{
    return std::make_shared<unixsocket::ScanningServerSocket>(_path, _mode, _scannerFactory);
}

unixsocket::ProcessControllerServerSocketPtr ThreatDetectorResources::createProcessControllerServerSocket(
    const std::string& _path,
    mode_t _mode,
    std::shared_ptr<unixsocket::IProcessControlMessageCallback> _processControlCallbacks
    )
{
    return std::make_shared<unixsocket::ProcessControllerServerSocket>(_path, _mode, _processControlCallbacks);
}

unixsocket::IProcessControlMessageCallbackPtr ThreatDetectorResources::createThreatDetectorCallBacks(
    ISophosThreatDetectorMainPtr _threatDetectorMain
    )
{
    return std::make_shared<ThreatDetectorControlCallback>(_threatDetectorMain);
}