// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/IThreatScanner.h"
#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"
#include "scan_messages/ScanResponse.h"
#include <gmock/gmock.h>

class MockScanner : public threat_scanner::IThreatScanner
{
public:
    MOCK_METHOD(scan_messages::ScanResponse, scan, (datatypes::AutoFd&, const scan_messages::ScanRequest& info));
    MOCK_METHOD(
        scan_messages::MetadataRescanResponse,
        metadataRescan,
        (const scan_messages::MetadataRescan&),
        (override));

    MOCK_METHOD(std::string, susiErrorToReadableError, (const std::string& filePath, const std::string& susiError));
};
class MockScannerFactory : public threat_scanner::IThreatScannerFactory
{
public:
    MOCK_METHOD(threat_scanner::IThreatScannerPtr, createScanner, (bool scanArchives, bool scanImages, bool detectPUAs));
    MOCK_METHOD(bool, update, ());
    MOCK_METHOD(bool, reload, ());
    MOCK_METHOD(void, shutdown, ());
    MOCK_METHOD(bool, susiIsInitialized, ());
    MOCK_METHOD(bool, updateSusiConfig, ());
    MOCK_METHOD(bool, detectPUAsEnabled, ());
    MOCK_METHOD(void, loadSusiSettingsIfRequired, (), (override));
};
