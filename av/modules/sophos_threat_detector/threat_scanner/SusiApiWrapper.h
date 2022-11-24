// Copyright (c) 2022 , Sophos Limited.  All rights reserved.

#pragma once

#include "ISusiApiWrapper.h"
#include "SusiTypes.h"

namespace threat_scanner
{
    class SusiApiWrapper : public ISusiApiWrapper
    {
        SusiResult SUSI_Initialize(const char* configJson, SusiCallbackTable* callbacks) override;
        SusiResult SUSI_Terminate() override;
        SusiResult SUSI_UpdateGlobalConfiguration(const char* configJson) override;
        SusiResult SUSI_Install(const char* installSourceFolder, const char* installDestination) override;
        SusiResult SUSI_Update(const char* updateSourceFolder) override;
        SusiResult SUSI_SetLogCallback(const SusiLogCallback* callback) override;
        SusiResult SUSI_GetVersion(SusiVersionResult** versionResult) override;
        SusiResult SUSI_FreeVersionResult(SusiVersionResult* versionResult) override;
    };
} // namespace threat_scanner
