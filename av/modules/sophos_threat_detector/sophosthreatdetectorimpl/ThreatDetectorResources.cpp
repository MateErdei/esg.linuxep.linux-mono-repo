// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Reloader.h"
#include "SafeStoreRescanWorker.h"
#include "ThreatDetectorResources.h"
#include "ThreatDetectorControlCallback.h"

#include "common/PidLockFile.h"
#include "common/signals/SigTermMonitor.h"
#include "common/signals/SigUSR1Monitor.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ShutdownTimer.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatReporter.h"
#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteServerSocket.h"

#include "Common/SystemCallWrapper/SystemCallWrapperFactory.h"

using namespace sspl::sophosthreatdetectorimpl;

Common::SystemCallWrapper::ISystemCallWrapperSharedPtr ThreatDetectorResources::createSystemCallWrapper()
{
    auto sysCallFact = Common::SystemCallWrapper::SystemCallWrapperFactory();
    return sysCallFact.createSystemCallWrapper();
}

common::signals::ISignalHandlerSharedPtr ThreatDetectorResources::createSigTermHandler(bool restartSyscalls)
{
    return std::make_shared<common::signals::SigTermMonitor>(restartSyscalls);
}

common::signals::ISignalHandlerSharedPtr ThreatDetectorResources::createUsr1Monitor(common::signals::IReloadablePtr reloadable)
{
    return std::make_shared<common::signals::SigUSR1Monitor>(reloadable);
}

std::shared_ptr<common::signals::IReloadable> ThreatDetectorResources::createReloader(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
{
    return std::make_shared<Reloader>(scannerFactory);
}

common::IPidLockFileSharedPtr ThreatDetectorResources::createPidLockFile(const std::string& path)
{
    return std::make_shared<common::PidLockFile>(path);
}

ISafeStoreRescanWorkerPtr ThreatDetectorResources::createSafeStoreRescanWorker(const sophos_filesystem::path& safeStoreRescanSocket)
{
    return std::make_shared<SafeStoreRescanWorker>(safeStoreRescanSocket);
}

threat_scanner::IThreatReporterSharedPtr ThreatDetectorResources::createThreatReporter(const sophos_filesystem::path socketPath)
{
    return std::make_shared<ThreatReporter>(socketPath);
}

threat_scanner::IScanNotificationSharedPtr ThreatDetectorResources::createShutdownTimer(const sophos_filesystem::path configPath)
{
    return std::make_shared<ShutdownTimer>(configPath);
}

threat_scanner::IThreatScannerFactorySharedPtr ThreatDetectorResources::createSusiScannerFactory(
    threat_scanner::IThreatReporterSharedPtr reporter,
    threat_scanner::IScanNotificationSharedPtr shutdownTimer,
    threat_scanner::IUpdateCompleteCallbackPtr updateCompleteCallback)
{
    return std::make_shared<threat_scanner::SusiScannerFactory>(reporter, shutdownTimer, updateCompleteCallback);
}

unixsocket::updateCompleteSocket::UpdateCompleteServerSocketPtr ThreatDetectorResources::createUpdateCompleteNotifier
    (const sophos_filesystem::path serverPath, mode_t mode)
{
    return std::make_shared<unixsocket::updateCompleteSocket::UpdateCompleteServerSocket>(serverPath, mode);
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

unixsocket::IProcessControlMessageCallbackPtr ThreatDetectorResources::createThreatDetectorCallBacks(
    ISophosThreatDetectorMain& threatDetectorMain
    )
{
    return std::make_shared<ThreatDetectorControlCallback>(threatDetectorMain);
}

std::shared_ptr<unixsocket::MetadataRescanServerSocket> ThreatDetectorResources::createMetadataRescanServerSocket(
    const std::string& path,
    mode_t mode,
    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
{
    auto metadataRescanServer = std::make_shared<unixsocket::MetadataRescanServerSocket>(path, mode, scannerFactory);
    return metadataRescanServer;
}
