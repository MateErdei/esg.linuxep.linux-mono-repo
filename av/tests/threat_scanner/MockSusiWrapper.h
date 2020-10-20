/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_threat_detector/threat_scanner/ISusiWrapper.h"

#include <gmock/gmock.h>

#include <utility>

class MockSusiWrapper : public threat_scanner::ISusiWrapper
{
public:
    MockSusiWrapper()
    {
    }

    explicit MockSusiWrapper(std::string scannerConfig)
    : m_scannerConfig(std::move(scannerConfig))
    {
    }

    MOCK_METHOD4(scanFile, SusiResult(
            const char* metaData,
            const char* filename,
            datatypes::AutoFd& fd,
            SusiScanResult** scanResult));

    MOCK_METHOD1(freeResult, void(SusiScanResult* scanResult));

    MOCK_METHOD1(update, bool(const std::string&));

    std::string m_scannerConfig;
};