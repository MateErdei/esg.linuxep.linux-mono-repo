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

    bool SusiSettings::operator==(const SusiSettings& other) const noexcept
    {
        return m_susiAllowListSha256 == other.m_susiAllowListSha256 &&
               m_susiSxlLookupEnabled == other.m_susiSxlLookupEnabled;
    }

    bool SusiSettings::operator!=(const SusiSettings& other) const noexcept
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
                LOGDEBUG("SUSI Settings JSON file does not exist");
                return false;
            }

            auto settingsJsonContent = fs->readFile(threatDetectorSettingsPath);
            if (settingsJsonContent.empty())
            {
                LOGDEBUG("SUSI Settings JSON file exists but contents is empty");
                return false;
            }
            json parsedConfig = json::parse(settingsJsonContent);

            if (parsedConfig.contains(ENABLED_SXL_LOOKUP_KEY))
            {
                m_susiSxlLookupEnabled = parsedConfig[ENABLED_SXL_LOOKUP_KEY];
            }

            if (parsedConfig.contains(SHA_ALLOW_LIST_KEY))
            {
                m_susiAllowListSha256 = parsedConfig[SHA_ALLOW_LIST_KEY].get<std::vector<std::string>>();
            }

            LOGDEBUG("Loaded Threat Detector SUSI settings");
            return true;
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Could not read Threat Detector SUSI settings: " << e.what());
        }
        catch (const json::parse_error& e)
        {
            LOGERROR("Failed to load Threat Detector SUSI settings JSON due to parse error, reason: " << e.what());
        }
        catch (const json::out_of_range& e)
        {
            LOGERROR("Failed to load Threat Detector SUSI settings JSON due to out of range error, reason: " << e.what());
        }
        catch (const json::type_error& e)
        {
            LOGERROR("Failed to load Threat Detector SUSI settings JSON due to type error, reason: " << e.what());
        }
        catch (const json::other_error& e)
        {
            LOGERROR("Failed to load Threat Detector SUSI settings JSON, reason: " << e.what());
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
            LOGERROR("Failed to save Threat Detector SUSI settings");
        }
    }

    std::string SusiSettings::serialise() const
    {
        nlohmann::json settings;
        settings[ENABLED_SXL_LOOKUP_KEY] = m_susiSxlLookupEnabled;
        settings[SHA_ALLOW_LIST_KEY] = m_susiAllowListSha256;
        return settings.dump();
    }

    bool SusiSettings::isAllowListed(const std::string& threatChecksum) const noexcept
    {
        std::scoped_lock scopedLock(m_accessMutex);
        return std::find(m_susiAllowListSha256.begin(), m_susiAllowListSha256.end(), threatChecksum) != m_susiAllowListSha256.end();
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

    const AllowList& SusiSettings::accessAllowList() const noexcept
    {
        return m_susiAllowListSha256;
    }
} // namespace common::ThreatDetector
