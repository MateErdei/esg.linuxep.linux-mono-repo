/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "scan_messages/ScanResponse.h"
#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"

#include "unixsocket/BaseServerConnectionThread.h"
#include "unixsocket/IMessageCallback.h"

#include "Common/Threads/AbstractThread.h"

#include <cstdint>
#include <string>

namespace unixsocket
{
    class ScanningServerConnectionThread : public BaseServerConnectionThread
    {
    public:
        ScanningServerConnectionThread(const ScanningServerConnectionThread&) = delete;
        ScanningServerConnectionThread& operator=(const ScanningServerConnectionThread&) = delete;
        explicit ScanningServerConnectionThread(
                int fd,
                threat_scanner::IThreatScannerFactorySharedPtr scannerFactory);
        void run() override;

    private:
        datatypes::AutoFd m_fd;
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
    };
}


