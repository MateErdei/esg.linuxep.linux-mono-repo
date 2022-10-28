// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/ISystemCallWrapper.h"

namespace sspl::sophosthreatdetectorimpl
{
    class IThreatDetectorResources
    {
    public:
        virtual ~IThreatDetectorResources() = default;

        virtual datatypes::ISystemCallWrapperSharedPtr createSystemCallWrapper() = 0;

       /* common::signals::SigTermMonitor createSigTermMonitor(bool restartSyscalls);
        common::PidLockFile createLockFile(fs::path path);
        //Will create syscallsfactory
        threat_scanner::IThreatReporterSharedPtr getThreatReporter(something about a socket )
        threat_scanner::IScanNotificationSharedPtr getShutdownTimer(something about threat_detector_config)
        unixsocket::updateCompleteSocket::UpdateCompleteServerSocket(path, permissions)
        threat_scanner::SusiScannerFactory
        Reloader()
        ScanningServerSocket
        SigUSR1Monitor(reloader)
        ThreatDetectorControlCallbacks*/

        //Need other way to do attempt_dns_query
        //Need other way to do copyRequiredFiles
        //Need other way to do copy_etc_files_for_dns


        //move remove_shutdown_notice_file to filesystem mock


    };
    using IThreatDetectorResourcesUniquePtr = std::unique_ptr<IThreatDetectorResources>;
}