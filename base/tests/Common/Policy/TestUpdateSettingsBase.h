// Copyright 2023 Sophos All rights reserved.

#pragma once

#include "Common/Policy/UpdateSettings.h"

#include "tests/Common/Helpers/MemoryAppender.h"

namespace
{
    class TestUpdateSettingsBase : public MemoryAppenderUsingTests
    {
    public:
        TestUpdateSettingsBase() : MemoryAppenderUsingTests("Policy") {}

        static Common::Policy::UpdateSettings getValidUpdateSettings()
        {
            Common::Policy::UpdateSettings validSettings;
            validSettings.setSophosLocationURLs({"http://really_sophos.info"});
            validSettings.setCredentials({"username", "password"});
            validSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
            validSettings.setFeatures({"CORE"});
            return validSettings;
        }
    };
}
