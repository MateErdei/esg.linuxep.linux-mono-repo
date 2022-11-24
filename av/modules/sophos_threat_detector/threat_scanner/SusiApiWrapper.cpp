// Copyright (c) 2022 , Sophos Limited.  All rights reserved.
#include "SusiApiWrapper.h"

#include "Susi.h"

namespace threat_scanner
{
    SusiResult SusiApiWrapper::SUSI_Initialize(const char* configJson, SusiCallbackTable* callbacks)
    {
        return ::SUSI_Initialize(configJson, callbacks);
    }

    SusiResult SusiApiWrapper::SUSI_Terminate()
    {
        return ::SUSI_Terminate();
    }

    SusiResult SusiApiWrapper::SUSI_UpdateGlobalConfiguration(const char* configJson)
    {
        return ::SUSI_UpdateGlobalConfiguration(configJson);
    }

    SusiResult SusiApiWrapper::SUSI_Install(const char* installSourceFolder, const char* installDestination)
    {
        return ::SUSI_Install(installSourceFolder, installDestination);
    }

    SusiResult SusiApiWrapper::SUSI_Update(const char* updateSourceFolder)
    {
        return ::SUSI_Update(updateSourceFolder);
    }

    SusiResult SusiApiWrapper::SUSI_SetLogCallback(const SusiLogCallback* callback)
    {
        return ::SUSI_SetLogCallback(callback);
    }

    SusiResult SusiApiWrapper::SUSI_GetVersion(SusiVersionResult** versionResult)
    {
        return ::SUSI_GetVersion(versionResult);
    }

    SusiResult SusiApiWrapper::SUSI_FreeVersionResult(SusiVersionResult* versionResult)
    {
        return ::SUSI_FreeVersionResult(versionResult);
    }
    
} // namespace threat_scanner