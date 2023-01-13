// Copyright 2020-2023, Sophos Limited.  All rights reserved.

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#pragma once

#include "IThreatDetectorResources.h"
#include "Reloader.h"
#include "SafeStoreRescanWorker.h"

#include "datatypes/SystemCallWrapperFactory.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteServerSocket.h"

#include "common/ThreatDetector/SusiSettings.h"

namespace sspl::sophosthreatdetectorimpl
{
    class SophosThreatDetectorMain : public ISophosThreatDetectorMain
    {
    public:
        int sophos_threat_detector_main();

        /**
         * Calls SusiGlobalHandler::reload and SusiGlobalHandler::susiIsInitialized()
         * both functions try to ensure thread safety
         */
        void reloadSUSIGlobalConfiguration() override;
        void shutdownThreatDetector() override;

    private:
        std::shared_ptr<common::signals::IReloadable> m_reloader;
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
        datatypes::ISystemCallWrapperSharedPtr m_sysCallWrapper;
        Common::Threads::NotifyPipe m_systemFileRestartTrigger;
        std::shared_ptr<ISafeStoreRescanWorker> m_safeStoreRescanWorker;
        unixsocket::updateCompleteSocket::UpdateCompleteServerSocketPtr m_updateCompleteNotifier;

        int dropCapabilities();
        int lockCapabilities();
        void attempt_dns_query();

    TEST_PUBLIC:
        int inner_main(const IThreatDetectorResourcesSharedPtr& resources);


    };
} // namespace sspl::sophosthreatdetectorimpl
