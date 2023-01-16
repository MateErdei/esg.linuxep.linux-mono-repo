// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IThreatDetectorResources.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ThreatDetectorResources : public IThreatDetectorResources
    {
        public:
            datatypes::ISystemCallWrapperSharedPtr createSystemCallWrapper() override;

            common::signals::ISignalHandlerSharedPtr createSigTermHandler(bool restartSyscalls) override;
            common::signals::ISignalHandlerSharedPtr createUsr1Monitor(common::signals::IReloadablePtr reloadable) override;
            std::shared_ptr<common::signals::IReloadable>  createReloader(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory) override;

            common::IPidLockFileSharedPtr createPidLockFile(const std::string& path) override;

            ISafeStoreRescanWorkerPtr createSafeStoreRescanWorker(const sophos_filesystem::path& safeStoreRescanSocket) override;

            threat_scanner::IThreatReporterSharedPtr createThreatReporter(const sophos_filesystem::path socketPath) override;

            threat_scanner::IScanNotificationSharedPtr createShutdownTimer(const sophos_filesystem::path configPath) override;

            threat_scanner::IThreatScannerFactorySharedPtr createSusiScannerFactory(
                threat_scanner::IThreatReporterSharedPtr reporter,
                threat_scanner::IScanNotificationSharedPtr shutdownTimer,
                threat_scanner::IUpdateCompleteCallbackPtr updateCompleteCallback) override;

            unixsocket::updateCompleteSocket::UpdateCompleteServerSocketPtr createUpdateCompleteNotifier
                (const sophos_filesystem::path serverPath, mode_t mode) override;

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
                ISophosThreatDetectorMain& threatDetectorMain
                ) override;
    };
}