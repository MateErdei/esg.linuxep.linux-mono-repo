// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "PolicyParseException.h"
#include "UpdateSettings.h"

namespace Common::Policy
{
    class UpdatePolicySerialisationException : public PolicyParseException
    {
    public:
        using PolicyParseException::PolicyParseException;
    };

    class SerialiseUpdateSettings
    {
    public:
        static UpdateSettings fromJsonSettings(const std::string& settingsString);
        static std::string toJsonSettings(const UpdateSettings& configurationData);
    };
} // namespace Common::Policy
