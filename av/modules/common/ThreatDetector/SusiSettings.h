// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "common/Exclusion.h"

#include <mutex>
#include <string>
#include <vector>

namespace common::ThreatDetector
{
    using AllowList = std::vector<std::string>;
    using AllowListPath = std::vector<Exclusion>;
    using PuaApprovedList = std::vector<std::string>;
    constexpr bool SXL_DEFAULT = false;  // Default to false until we have a policy

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
        bool isAllowListedSha256(const std::string& threatChecksum) const;
        bool isAllowListedPath(const std::string& threatPath) const;
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

        std::string getSxlUrl() const;
        void setSxlUrl(const std::string&);
        /**
         * Determines if the SXL4 URL is valid.
         * Must be UTF-8
         * Must be AlphaNumeric + .
         * Must be small enough
         * @param url
         * @return true if URL is valid
         */
        static bool isSxlUrlValid(const std::string& url);

    private:
        void processRawPathAllowList();

        // Susi can access the allow-list while we're changing it, so make sure it's thread safe.
        mutable std::mutex m_accessMutex;

        AllowList m_susiAllowListSha256;
        AllowList m_susiAllowListPathRaw; //For checking against updated policy & json operations
        AllowListPath m_susiAllowListPath; //For checking against detections
        PuaApprovedList m_susiPuaApprovedList;
        std::string sxlUrl_;
        bool m_susiSxlLookupEnabled = SXL_DEFAULT;
        bool m_machineLearningEnabled = true;

        [[nodiscard]] std::string serialise() const;
        static constexpr auto ENABLED_SXL_LOOKUP_KEY = "enableSxlLookup";
        static constexpr auto SHA_ALLOW_LIST_KEY = "shaAllowList";
        static constexpr auto PATH_ALLOW_LIST_KEY = "pathAllowList";
        static constexpr auto MACHINE_LEARNING_KEY = "machineLearning";
        static constexpr auto PUA_APPROVED_LIST_KEY = "puaApprovedList";
        static constexpr auto SXL_URL_KEY = "sxlUrl"; // empty means disable SxlLookup regardless of ENABLED_SXL_LOOKUP_KEY
    };
} // namespace common::ThreatDetector