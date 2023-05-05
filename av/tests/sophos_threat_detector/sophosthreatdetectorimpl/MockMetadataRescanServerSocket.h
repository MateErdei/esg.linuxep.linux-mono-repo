// Copyright 2023 Sophos Limited. All rights reserved.

#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"
#include "unixsocket/metadataRescanSocket/MetadataRescanServerSocket.h"

#include <gmock/gmock.h>

namespace
{
    class MockMetadataRescanServerSocket : public unixsocket::MetadataRescanServerSocket
    {
    public:
        MockMetadataRescanServerSocket(
            const sophos_filesystem::path& serverPath,
            mode_t mode,
            std::shared_ptr<threat_scanner::IThreatScannerFactory> scannerFactory) :
            unixsocket::MetadataRescanServerSocket(serverPath, mode, scannerFactory)
        {
            ON_CALL(*this, run).WillByDefault(InvokeWithoutArgs([this]() { MetadataRescanServerSocket::run(); }));
        };

        MOCK_METHOD(TPtr, makeThread, (datatypes::AutoFd & fd), (override));
        MOCK_METHOD(void, logMaxConnectionsError, (), (override));

        MOCK_METHOD(void, run, (), (override));
    };
} // namespace
