// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/ISusiWrapper.h"

#include <gmock/gmock.h>

#include <utility>

class MockSusiWrapper : public threat_scanner::ISusiWrapper
{
public:
    MockSusiWrapper() = default;

    explicit MockSusiWrapper(std::string scannerConfig) : m_scannerConfig(std::move(scannerConfig))
    {
    }

    MOCK_METHOD(
        SusiResult,
        scanFile,
        (const char* metaData, const char* filename, datatypes::AutoFd& fd, SusiScanResult** scanResult),
        (override));

    MOCK_METHOD(SusiResult, metadataRescan, (const char* metaData, SusiScanResult** scanResult), (override));

    MOCK_METHOD(void, freeResult, (SusiScanResult * scanResult), (override));

    std::string m_scannerConfig;
};