// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once
#include <string>
#include <vector>

namespace common::ThreatDetector
{
    class SusiSettings
    {
    public:
        SusiSettings() = default;
        explicit SusiSettings(const std::string& jsonSettingsPath);
        bool load(const std::string& threatDetectorSettingsPath);
        void saveSettings(const std::string& path, mode_t permissions) const;
        [[nodiscard]] std::string serialise() const;
        bool operator==(const SusiSettings& other) const
        {
            return m_susiAllowListSha256 == other.m_susiAllowListSha256 &&
                   m_susiSxlLookupEnabled == other.m_susiSxlLookupEnabled;
        }

        bool m_susiSxlLookupEnabled = true;
        std::vector<std::string> m_susiAllowListSha256;

    private:
        static constexpr auto ENABLED_SXL_LOOKUP_KEY = "enableSxlLookup";
        static constexpr auto SHA_ALLOW_LIST_KEY = "shaAllowList";
    };
} // namespace common::ThreatDetector