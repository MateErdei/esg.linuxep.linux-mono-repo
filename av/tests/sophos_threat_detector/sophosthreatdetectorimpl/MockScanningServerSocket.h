// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockScanningServerSocket : public unixsocket::ScanningServerSocket
    {
    public:
        MockScanningServerSocket(const sophos_filesystem::path serverPath, mode_t mode, threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
            : unixsocket::ScanningServerSocket(serverPath, mode, scannerFactory)
                {
                };

        MOCK_METHOD(TPtr, makeThread, (datatypes::AutoFd& fd), (override));
        MOCK_METHOD(void, logMaxConnectionsError, (), (override));
    };
}
