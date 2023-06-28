// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "UpdateSettings.h"

namespace Common::Policy
{
    class SerialiseUpdateSettings
    {
    public:
        static UpdateSettings fromJsonSettings(const std::string& settingsString);
        static std::string toJsonSettings(const UpdateSettings& configurationData);
    };
}
