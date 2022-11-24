//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "modules/sophos_threat_detector/threat_scanner/IThreatScanner.h"
#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"

#include <gmock/gmock.h>

class MockScanner : public threat_scanner::IThreatScanner
{
public:
    MOCK_METHOD(scan_messages::ScanResponse, scan, (datatypes::AutoFd&, const std::string&, int64_t,
                                                    const std::string& userID));

    MOCK_METHOD(std::string, susiErrorToReadableError, (const std::string& filePath, const std::string& susiError));
};

class MockScannerFactory : public threat_scanner::IThreatScannerFactory
{
public:
    MOCK_METHOD(threat_scanner::IThreatScannerPtr, createScanner, (bool scanArchives, bool scanImages));

    MOCK_METHOD(bool, update, ());
    MOCK_METHOD(bool, reload, ());
    MOCK_METHOD(void, shutdown, ());
    MOCK_METHOD(bool, susiIsInitialized, ());
};