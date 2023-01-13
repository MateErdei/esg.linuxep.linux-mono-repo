// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#pragma once

#include "IThreatDetectorResources.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ThreatDetectorResources : public IThreatDetectorResources
    {
        public:
            datatypes::ISystemCallWrapperSharedPtr createSystemCallWrapper() override;

            common::signals::ISignalHandlerSharedPtr createSigTermHandler(bool _restartSyscalls) override;
            common::signals::ISignalHandlerSharedPtr createUsr1Monitor(common::signals::IReloadablePtr _reloadable) override;
            std::shared_ptr<common::signals::IReloadable>  createReloader(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory) override;

            common::IPidLockFileSharedPtr createPidLockFile(const std::string& _path) override;

            ISafeStoreRescanWorkerPtr createSafeStoreRescanWorker(const sophos_filesystem::path& safeStoreRescanSocket) override;

            threat_scanner::IThreatReporterSharedPtr createThreatReporter(const sophos_filesystem::path _socketPath) override;

            threat_scanner::IScanNotificationSharedPtr createShutdownTimer(const sophos_filesystem::path _configPath) override;

            threat_scanner::IThreatScannerFactorySharedPtr createSusiScannerFactory(
                threat_scanner::IThreatReporterSharedPtr _reporter,
                threat_scanner::IScanNotificationSharedPtr _shutdownTimer,
                threat_scanner::IUpdateCompleteCallbackPtr _updateCompleteCallback) override;

            unixsocket::updateCompleteSocket::UpdateCompleteServerSocketPtr createUpdateCompleteNotifier
                (const sophos_filesystem::path _serverPath, mode_t _mode) override;

            unixsocket::ScanningServerSocketPtr createScanningServerSocket(
                const std::string& path,
                mode_t mode,
                threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
                ) override;

            unixsocket::ProcessControllerServerSocketPtr createProcessControllerServerSocket(
                const std::string& path,
                mode_t mode,
                std::shared_ptr<unixsocket::IProcessControlMessageCallback> processControlCallbacks
                ) override;

            unixsocket::IProcessControlMessageCallbackPtr createThreatDetectorCallBacks(
                ISophosThreatDetectorMainPtr _threatDetectorMain
                ) override;
    };
}