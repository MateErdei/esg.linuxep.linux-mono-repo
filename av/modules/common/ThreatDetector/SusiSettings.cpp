// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SusiSettings.h"

#include "json.hpp"

#include "../Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

using namespace nlohmann;

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
               m_susiSxlLookupEnabled == other.m_susiSxlLookupEnabled;
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

            if (parsedConfig.contains(ENABLED_SXL_LOOKUP_KEY))
            {
                m_susiSxlLookupEnabled = parsedConfig[ENABLED_SXL_LOOKUP_KEY];

                // Added to match existing logging behaviour
                if (m_susiSxlLookupEnabled)
                {
                    LOGDEBUG("SXL Lookups will be enabled");
                }
                else
                {
                    LOGDEBUG("SXL Lookups will be disabled");
                }
            }

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
            LOGINFO("Saved Threat Detector SUSI settings");
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Failed to save Threat Detector SUSI settings: " << e.what());
        }
    }

    std::string SusiSettings::serialise() const
    {
        nlohmann::json settings;
        settings[ENABLED_SXL_LOOKUP_KEY] = m_susiSxlLookupEnabled;
        settings[SHA_ALLOW_LIST_KEY] = m_susiAllowListSha256;
        return settings.dump();
    }

    bool SusiSettings::isAllowListed(const std::string& threatChecksum) const
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return std::find(m_susiAllowListSha256.begin(), m_susiAllowListSha256.end(), threatChecksum) !=
               m_susiAllowListSha256.end();
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
} // namespace common::ThreatDetector
