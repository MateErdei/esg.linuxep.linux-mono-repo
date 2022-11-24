// Copyright (c) 2022 , Sophos Limited.  All rights reserved.

#pragma once

#include "redist/susi_build/include/SusiTypes.h"
#include "sophos_threat_detector/threat_scanner/ISusiApiWrapper.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace threat_scanner;

namespace
{
    class MockSusiApi : public ISusiApiWrapper
    {
    public:
        MockSusiApi() = default;

        MOCK_METHOD(SusiResult, SUSI_Initialize, (const char* configJson, SusiCallbackTable* callbacks), (override));
        MOCK_METHOD(SusiResult, SUSI_Terminate, (), (override));
        MOCK_METHOD(SusiResult, SUSI_UpdateGlobalConfiguration, (const char* configJson), (override));
        MOCK_METHOD(SusiResult, SUSI_Install, (const char* installSourceFolder, const char* installDestination), (override));
        MOCK_METHOD(SusiResult, SUSI_Update, (const char* updateSourceFolder), (override));
        MOCK_METHOD(SusiResult, SUSI_SetLogCallback, (const SusiLogCallback* callback), (override));
        MOCK_METHOD(SusiResult, SUSI_GetVersion, (SusiVersionResult** versionResult), (override));
        MOCK_METHOD(SusiResult, SUSI_FreeVersionResult, (SusiVersionResult* versionResult), (override));
    };
}
