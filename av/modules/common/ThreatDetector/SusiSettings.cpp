// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SusiSettings.h"

#include "json.hpp"

#include "../Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

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
               m_susiSxlLookupEnabled == other.m_susiSxlLookupEnabled &&
               m_oaPuaDetectionEnabled == other.m_oaPuaDetectionEnabled &&
               m_machineLearningEnabled == other.m_machineLearningEnabled;
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
                LOGINFO("Turning Live Protection on as default - no SUSI settings found");
                return false;
            }

            auto settingsJsonContent = fs->readFile(threatDetectorSettingsPath);
            if (settingsJsonContent.empty())
            {
                LOGDEBUG("SUSI Settings JSON file exists but contents are empty");
                return false;
            }
            json parsedConfig = json::parse(settingsJsonContent);

            m_susiSxlLookupEnabled = getBooleanFromJson(parsedConfig, ENABLED_SXL_LOOKUP_KEY, "SXL Lookups", true);
            m_machineLearningEnabled = getBooleanFromJson(parsedConfig, MACHINE_LEARNING_KEY, "Machine Learning", true);
            m_oaPuaDetectionEnabled = getBooleanFromJson(parsedConfig, OA_PUA_DETECTION_KEY, "OA PUA Detection", true);

            if (parsedConfig.contains(SHA_ALLOW_LIST_KEY))
            {
                m_susiAllowListSha256 = parsedConfig[SHA_ALLOW_LIST_KEY].get<std::vector<std::string>>();
                LOGDEBUG("Number of SHA256 allow-listed items: " << m_susiAllowListSha256.size());
            }

            LOGDEBUG("Loaded Threat Detector SUSI settings into SUSI settings object");
            return true;
        }
        catch (const std::exception& ex)
        {
            LOGWARN("Failed to load Threat Detector SUSI settings JSON, reason: " << ex.what());
            LOGINFO("Turning Live Protection on as default - could not read SUSI settings");
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
        settings[OA_PUA_DETECTION_KEY] = m_oaPuaDetectionEnabled;
        settings[SHA_ALLOW_LIST_KEY] = m_susiAllowListSha256;
        return settings.dump();
    }

    bool SusiSettings::isAllowListed(const std::string& threatChecksum) const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return std::find(m_susiAllowListSha256.cbegin(), m_susiAllowListSha256.cend(), threatChecksum) !=
               m_susiAllowListSha256.cend();
    }

    void SusiSettings::setAllowList(AllowList&& allowList) noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        m_susiAllowListSha256 = std::move(allowList);
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

    size_t SusiSettings::getAllowListSize() const noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiAllowListSha256.size();
    }

    AllowList SusiSettings::copyAllowList() const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiAllowListSha256;
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

    bool SusiSettings::isOaPuaDetectionEnabled() const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_oaPuaDetectionEnabled;
    }

    void SusiSettings::setOaPuaDetectionEnabled(bool enabled)
    {
        std::scoped_lock scopedLock(m_accessMutex);
        m_oaPuaDetectionEnabled = enabled;
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

    AllowList SusiSettings::copyPuaApprovedList() const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return m_susiPuaApprovedList;
    }
} // namespace common::ThreatDetector
