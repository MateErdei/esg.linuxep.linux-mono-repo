// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once
#include <mutex>
#include <string>
#include <vector>

namespace common::ThreatDetector
{
    using AllowList = std::vector<std::string>;
    using PuaApprovedList = std::vector<std::string>;

    class SusiSettings
    {
    public:
        SusiSettings() = default;
        explicit SusiSettings(const std::string& jsonSettingsPath);
        bool operator==(const SusiSettings& other) const;
        bool operator!=(const SusiSettings& other) const;

        bool load(const std::string& threatDetectorSettingsPath);
        void saveSettings(const std::string& path, mode_t permissions) const;

        // Allow listing
        bool isAllowListed(const std::string& threatChecksum) const;
        void setAllowListSha256(AllowList&& allowListBySha) noexcept;
        void setAllowListPath(AllowList&& allowListByPath) noexcept;
        size_t getAllowListSizeSha256() const noexcept;
        size_t getAllowListSizePath() const noexcept;
        AllowList copyAllowListSha256() const;
        AllowList copyAllowListPath() const;

        // SXL lookups
        bool isSxlLookupEnabled() const noexcept;
        void setSxlLookupEnabled(bool enabled) noexcept;

        // Machine Learning Enabled
        bool isMachineLearningEnabled() const;
        void setMachineLearningEnabled(bool enabled);

        // PUA Approved list
        bool isPuaApproved(const std::string& puaName) const;
        void setPuaApprovedList(PuaApprovedList&& approvedList) noexcept;
        PuaApprovedList copyPuaApprovedList() const;

    private:
        // Susi can access the allow-list while we're changing it, so make sure it's thread safe.
        mutable std::mutex m_accessMutex;

        AllowList m_susiAllowListSha256;
        AllowList m_susiAllowListPath;
        bool m_susiSxlLookupEnabled = true;
        bool m_machineLearningEnabled = true;
        PuaApprovedList m_susiPuaApprovedList;

        [[nodiscard]] std::string serialise() const;
        static constexpr auto ENABLED_SXL_LOOKUP_KEY = "enableSxlLookup";
        static constexpr auto SHA_ALLOW_LIST_KEY = "shaAllowList";
        static constexpr auto PATH_ALLOW_LIST_KEY = "pathAllowList";
        static constexpr auto MACHINE_LEARNING_KEY = "machineLearning";
        static constexpr auto PUA_APPROVED_LIST_KEY = "puaApprovedList";
    };
} // namespace common::ThreatDetector