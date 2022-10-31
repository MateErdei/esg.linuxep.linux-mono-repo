// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IThreatDetectorResources.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ThreatDetectorResources : public IThreatDetectorResources
    {
        public:
            datatypes::ISystemCallWrapperSharedPtr createSystemCallWrapper() override;

            common::signals::ISignalHandlerSharedPtr createSignalHandler(bool _restartSyscalls) override;

            common::IPidLockFileSharedPtr createPidLockFile(const std::string& _path) override;

            threat_scanner::IThreatReporterSharedPtr createThreatReporter(const sophos_filesystem::path _socketPath) override;

            threat_scanner::IScanNotificationSharedPtr createShutdownTimer(const sophos_filesystem::path _configPath) override;

            unixsocket::updateCompleteSocket::UpdateCompleteServerSocketPtr createUpdateCompleteNotifier
                (const sophos_filesystem::path _serverPath, mode_t _mode) override;

            threat_scanner::IThreatScannerFactorySharedPtr createSusiScannerFactory(
                threat_scanner::IThreatReporterSharedPtr _reporter,
                threat_scanner::IScanNotificationSharedPtr _shutdownTimer,
                threat_scanner::IUpdateCompleteCallbackPtr _updateCompleteCallback) override;
    };
}