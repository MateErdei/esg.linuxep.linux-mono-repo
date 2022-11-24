// Copyright (c) 2022 , Sophos Limited.  All rights reserved.

#pragma once

#include "SusiTypes.h"

namespace threat_scanner
{
    class ISusiApiWrapper
    {
    public:
        virtual ~ISusiApiWrapper() = default;

        virtual SusiResult SUSI_Initialize(const char* configJson, SusiCallbackTable* callbacks) = 0;
        virtual SusiResult SUSI_Terminate() = 0;
        virtual SusiResult SUSI_UpdateGlobalConfiguration(const char* configJson) = 0;
        virtual SusiResult SUSI_Install(const char* installSourceFolder, const char* installDestination) = 0;
        virtual SusiResult SUSI_Update(const char* updateSourceFolder) = 0;
        virtual SusiResult SUSI_SetLogCallback(const SusiLogCallback* callback) = 0;
        virtual SusiResult SUSI_GetVersion(SusiVersionResult** versionResult) = 0;
        virtual SusiResult SUSI_FreeVersionResult(SusiVersionResult* versionResult) = 0;
    };
}
