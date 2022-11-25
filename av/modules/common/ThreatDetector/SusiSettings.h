// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once
#include <string>
#include <vector>
#include <mutex>

namespace common::ThreatDetector
{
    using AllowList = std::vector<std::string>;

    class SusiSettings
    {
    public:
        SusiSettings() = default;
        explicit SusiSettings(const std::string& jsonSettingsPath);
        bool operator==(const SusiSettings& other) const noexcept;
        bool operator!=(const SusiSettings& other) const noexcept;

        bool load(const std::string& threatDetectorSettingsPath);
        void saveSettings(const std::string& path, mode_t permissions) const;

        // Allow listing
        bool isAllowListed(const std::string& threatChecksum) const noexcept;
        void setAllowList(AllowList&& allowList) noexcept;
        const AllowList& accessAllowList() const noexcept;

        // SXL lookups
        bool isSxlLookupEnabled() const noexcept;
        void setSxlLookupEnabled(bool enabled) noexcept;

    private:

        // Susi can access the allow-list while we're changing it, so make sure it's thread safe.
        mutable std::mutex m_accessMutex;

        AllowList m_susiAllowListSha256;
        bool m_susiSxlLookupEnabled = true;

        [[nodiscard]] std::string serialise() const;
        static constexpr auto ENABLED_SXL_LOOKUP_KEY = "enableSxlLookup";
        static constexpr auto SHA_ALLOW_LIST_KEY = "shaAllowList";
    };
} // namespace common::ThreatDetector