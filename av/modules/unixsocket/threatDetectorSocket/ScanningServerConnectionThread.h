/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "scan_messages/ScanResponse.h"
#include "threat_scanner/IThreatScannerFactory.h"

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
        explicit ScanningServerConnectionThread(int fd,
                std::shared_ptr<IMessageCallback> callback,
                std::shared_ptr<threat_scanner::IThreatScannerFactory> scannerFactory);
        void run() override;

    private:
        datatypes::AutoFd m_fd;
        std::shared_ptr<IMessageCallback> m_callback;
        std::shared_ptr<threat_scanner::IThreatScannerFactory> m_scannerFactory;
    };
}


