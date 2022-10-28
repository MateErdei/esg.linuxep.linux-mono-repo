// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#pragma once

#include "IThreatDetectorResources.h"
#include "Reloader.h"

#include "datatypes/SystemCallWrapperFactory.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteServerSocket.h"

namespace sspl::sophosthreatdetectorimpl
{
    class SophosThreatDetectorMain
    {
    public:
        int sophos_threat_detector_main();

        /**
         * Calls SusiGlobalHandler::reload and SusiGlobalHandler::susiIsInitialized()
         * both functions try to ensure thread safety
         */
        void reloadSUSIGlobalConfiguration();
        void shutdownThreatDetector();

    private:
        std::shared_ptr<Reloader> m_reloader;
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
        datatypes::ISystemCallWrapperSharedPtr m_sysCallWrapper;

    TEST_PUBLIC:
        int inner_main(IThreatDetectorResourcesUniquePtr resources);
        void attempt_dns_query();
    };
}
