// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ISafeStoreRescanWorker.h"
#include "ISophosThreatDetectorMain.h"
#include "common/IPidLockFile.h"
#include "common/signals/IReloadable.h"
#include "common/signals/ISignalHandlerBase.h"
#include "datatypes/ISystemCallWrapper.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_threat_detector/threat_scanner/IThreatReporter.h"
#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"
#include "sophos_threat_detector/threat_scanner/IScanNotification.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteServerSocket.h"

namespace sspl::sophosthreatdetectorimpl
{
    class IThreatDetectorResources
    {
    public:
        virtual ~IThreatDetectorResources() = default;

        virtual common::signals::ISignalHandlerSharedPtr createSigTermHandler(bool _restartSyscalls) = 0;
        virtual common::signals::ISignalHandlerSharedPtr createUsr1Monitor(common::signals::IReloadablePtr _reloadable) = 0;
        virtual std::shared_ptr<common::signals::IReloadable> createReloader(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory) = 0;

        virtual common::IPidLockFileSharedPtr createPidLockFile(const std::string& _path) = 0;

        virtual ISafeStoreRescanWorkerPtr createSafeStoreRescanWorker(const sophos_filesystem::path& safeStoreRescanSocket) = 0;

        virtual datatypes::ISystemCallWrapperSharedPtr createSystemCallWrapper() = 0;

        virtual threat_scanner::IThreatReporterSharedPtr createThreatReporter(const sophos_filesystem::path _socketPath) = 0;

        virtual threat_scanner::IScanNotificationSharedPtr createShutdownTimer(const sophos_filesystem::path _configPath) = 0;

        virtual threat_scanner::IThreatScannerFactorySharedPtr createSusiScannerFactory(
            threat_scanner::IThreatReporterSharedPtr _reporter,
            threat_scanner::IScanNotificationSharedPtr _shutdownTimer,
            threat_scanner::IUpdateCompleteCallbackPtr _updateCompleteCallback) = 0;

        virtual unixsocket::updateCompleteSocket::UpdateCompleteServerSocketPtr createUpdateCompleteNotifier
            (const sophos_filesystem::path _serverPath, mode_t _mode) = 0;

        virtual unixsocket::ScanningServerSocketPtr createScanningServerSocket(
            const std::string& _path,
            mode_t _mode,
            threat_scanner::IThreatScannerFactorySharedPtr _scannerFactory
            ) = 0;

        virtual unixsocket::ProcessControllerServerSocketPtr createProcessControllerServerSocket(
            const std::string& _path,
            mode_t _mode,
            std::shared_ptr<unixsocket::IProcessControlMessageCallback> _processControlCallbacks
            ) = 0;

        virtual unixsocket::IProcessControlMessageCallbackPtr createThreatDetectorCallBacks(
            ISophosThreatDetectorMainPtr _threatDetectorMain
            ) = 0;
    };
    using IThreatDetectorResourcesSharedPtr = std::shared_ptr<IThreatDetectorResources>;
}