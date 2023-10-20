// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SusiSettings.h"

#include <nlohmann/json.hpp>

#include "common/Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/StringUtils.h"

using namespace nlohmann;

namespace
{
    bool getBooleanFromJson(const json& parsedConfig, const std::string& key, const std::string& name, bool defaultValue)
    {
        if (!parsedConfig.contains(key))
        {
            return defaultValue;
        }
        bool value = parsedConfig[key];

        // Added to match existing logging behaviour
        if (value)
        {
            LOGDEBUG(name << " will be enabled");
        }
        else
        {
            LOGDEBUG(name << " will be disabled");
        }
        return value;
    }

#ifdef LOGGING_ENABLED
    // Only used in log lines
    const char* strFromBool(bool v)
    {
        return v ? "on" : "off";
    }

    const char* sxl_default_str()
    {
        return strFromBool(common::ThreatDetector::SXL_DEFAULT);
    }
#endif
}

namespace common::ThreatDetector
{
    SusiSettings::SusiSettings(const std::string& jsonSettingsPath)
    {
        load(jsonSettingsPath);
    }

    bool SusiSettings::operator==(const SusiSettings& other) const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiAllowListSha256 == other.m_susiAllowListSha256 &&
               m_susiAllowListPath == other.m_susiAllowListPath &&
               m_susiAllowListPathRaw == other.m_susiAllowListPathRaw &&
               m_susiSxlLookupEnabled == other.m_susiSxlLookupEnabled &&
               m_susiPuaApprovedList == other.m_susiPuaApprovedList &&
               m_machineLearningEnabled == other.m_machineLearningEnabled &&
               sxlUrl_ == other.sxlUrl_;
    }

    bool SusiSettings::operator!=(const SusiSettings& other) const
    {
        return !(*this == other);
    }

    bool SusiSettings::load(const std::string& threatDetectorSettingsPath)
    {
        std::scoped_lock scopedLock(m_accessMutex);
        try
        {
            auto fs = Common::FileSystem::fileSystem();
            if (!fs->isFile(threatDetectorSettingsPath))
            {
                LOGINFO("Turning Live Protection " << sxl_default_str() << " as default - no SUSI settings found");
                return false;
            }

            auto settingsJsonContent = fs->readFile(threatDetectorSettingsPath);
            if (settingsJsonContent.empty())
            {
                LOGDEBUG("SUSI Settings JSON file exists but contents are empty");
                return false;
            }
            json parsedConfig = json::parse(settingsJsonContent);

            m_susiSxlLookupEnabled = getBooleanFromJson(parsedConfig, ENABLED_SXL_LOOKUP_KEY, "SXL Lookups", false);
            m_machineLearningEnabled = getBooleanFromJson(parsedConfig, MACHINE_LEARNING_KEY, "Machine Learning", true);

            if (parsedConfig.contains(SHA_ALLOW_LIST_KEY))
            {
                m_susiAllowListSha256 = parsedConfig[SHA_ALLOW_LIST_KEY].get<std::vector<std::string>>();
                LOGDEBUG("Number of SHA256 allow-listed items: " << m_susiAllowListSha256.size());
            }

            if (parsedConfig.contains(PATH_ALLOW_LIST_KEY))
            {
                m_susiAllowListPathRaw = parsedConfig[PATH_ALLOW_LIST_KEY].get<std::vector<std::string>>();
                m_susiAllowListPathRaw.erase
                    (std::remove_if(m_susiAllowListPathRaw.begin(), m_susiAllowListPathRaw.end(), [](auto item) {return item.front() != '/' && item.front() != '*';})
                                                 , m_susiAllowListPathRaw.end());
                processRawPathAllowList();
                LOGDEBUG("Number of Path allow-listed items: " << m_susiAllowListPath.size());
            }

            if (parsedConfig.contains(PUA_APPROVED_LIST_KEY))
            {
                m_susiPuaApprovedList = parsedConfig[PUA_APPROVED_LIST_KEY].get<std::vector<std::string>>();
                LOGDEBUG("Number of approved PUA items: " << m_susiPuaApprovedList.size());
            }

            sxlUrl_ = parsedConfig[SXL_URL_KEY].get<std::string>();
            LOGDEBUG("SXL URL: " << sxlUrl_);

            LOGDEBUG("Loaded Threat Detector SUSI settings into SUSI settings object");
            return true;
        }
        catch (const std::exception& ex)
        {
            LOGWARN("Failed to load Threat Detector SUSI settings JSON, reason: " << ex.what());
            LOGINFO("Turning Live Protection " << sxl_default_str() << " as default - could not read SUSI settings");
        }

        return false;
    }

    void SusiSettings::saveSettings(const std::string& path, mode_t permissions) const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        try
        {
            auto fs = Common::FileSystem::fileSystem();
            auto pluginTempPath = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
            fs->writeFileAtomically(path, serialise(), pluginTempPath, permissions);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Failed to save Threat Detector SUSI settings: " << e.what());
        }
    }

    std::string SusiSettings::serialise() const
    {
        nlohmann::json settings;
        settings[MACHINE_LEARNING_KEY] = m_machineLearningEnabled;
        settings[ENABLED_SXL_LOOKUP_KEY] = m_susiSxlLookupEnabled;
        settings[PATH_ALLOW_LIST_KEY] = m_susiAllowListPathRaw;
        settings[SHA_ALLOW_LIST_KEY] = m_susiAllowListSha256;
        settings[PUA_APPROVED_LIST_KEY] = m_susiPuaApprovedList;
        settings[SXL_URL_KEY] = sxlUrl_;
        return settings.dump();
    }

    bool SusiSettings::isAllowListedSha256(const std::string& threatChecksum) const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return std::find(m_susiAllowListSha256.cbegin(), m_susiAllowListSha256.cend(), threatChecksum) !=
                          m_susiAllowListSha256.cend();
    }

    bool SusiSettings::isAllowListedPath(const std::string& threatFile) const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return std::find_if(m_susiAllowListPath.cbegin(), m_susiAllowListPath.cend(), [&](const Exclusion& pathAllowed){
                                return pathAllowed.appliesToPath(threatFile);
                            }) != m_susiAllowListPath.cend();
    }

    void SusiSettings::setAllowListSha256(AllowList&& allowListBySha) noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        m_susiAllowListSha256 = std::move(allowListBySha);
    }

    void SusiSettings::setAllowListPath(AllowList&& allowListByPath) noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        m_susiAllowListPathRaw = std::move(allowListByPath);
        processRawPathAllowList();
    }

    bool SusiSettings::isSxlLookupEnabled() const noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiSxlLookupEnabled;
    }

    void SusiSettings::setSxlLookupEnabled(bool enabled) noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        m_susiSxlLookupEnabled = enabled;
    }

    size_t SusiSettings::getAllowListSizeSha256() const noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiAllowListSha256.size();
    }

    size_t SusiSettings::getAllowListSizePath() const noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiAllowListPath.size();
    }

    AllowList SusiSettings::copyAllowListSha256() const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiAllowListSha256;
    }

    AllowList SusiSettings::copyAllowListPath() const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiAllowListPathRaw;
    }

    bool SusiSettings::isMachineLearningEnabled() const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_machineLearningEnabled;
    }

    void SusiSettings::setMachineLearningEnabled(bool enabled)
    {
        std::scoped_lock scopedLock(m_accessMutex);
        m_machineLearningEnabled = enabled;
    }

    void SusiSettings::setPuaApprovedList(PuaApprovedList&& approvedList) noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        m_susiPuaApprovedList = approvedList;
    }

    bool SusiSettings::isPuaApproved(const std::string& puaName) const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return std::find(m_susiPuaApprovedList.cbegin(), m_susiPuaApprovedList.cend(), puaName) !=
               m_susiPuaApprovedList.cend();
    }

    PuaApprovedList SusiSettings::copyPuaApprovedList() const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiPuaApprovedList;
    }

    void SusiSettings::processRawPathAllowList()
    {
        std::vector<Exclusion> allowListPath;
        for (const auto& rawPathItr : m_susiAllowListPathRaw)
        {
            allowListPath.emplace_back(rawPathItr);
        }
        m_susiAllowListPath.swap(allowListPath);
    }

    std::string SusiSettings::getSxlUrl() const
    {
        return sxlUrl_;
    }

    void SusiSettings::setSxlUrl(const std::string& url)
    {
        sxlUrl_ = url;
    }

    bool SusiSettings::isSxlUrlValid(const std::string& url)
    {
        if (url.size() > 1024)
        {
            return false;
        }
        if (!Common::UtilityImpl::StringUtils::startswith(url, "https://"))
        {
            return false;
        }
        auto host = url.substr(9);
        auto pos = host.find_first_not_of(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789./");

        return pos == std::string::npos;
    }

} // namespace common::ThreatDetector
