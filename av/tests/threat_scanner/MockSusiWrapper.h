/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_threat_detector/threat_scanner/ISusiWrapper.h"

#include <gmock/gmock.h>

class MockSusiWrapper : public ISusiWrapper
{
public:
    MockSusiWrapper(const std::string& runtimeConfig, const std::string& scannerConfig)
    : m_runtimeConfig(std::move(runtimeConfig))
    , m_scannerConfig(std::move(scannerConfig))
    {
    }

    MOCK_METHOD3(scanFile, SusiResult(
            const char* metaData,
            const char* filename,
            SusiScanResult** scanResult));

    MOCK_METHOD1(freeResult, void(SusiScanResult* scanResult));

    std::string m_runtimeConfig;
    std::string m_scannerConfig;
};