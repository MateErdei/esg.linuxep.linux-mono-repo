/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Reloader.h"

#include "unixsocket/updateCompleteSocket/UpdateCompleteServerSocket.h"
#include "unixsocket/updateCompleteSocket/IUpdateCompleteCallback.h"

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
        int inner_main();

        std::shared_ptr<Reloader> m_reloader;
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
        std::shared_ptr<unixsocket::updateCompleteSocket::UpdateCompleteServerSocket> m_updateCompleteNotifier;
    };
}
