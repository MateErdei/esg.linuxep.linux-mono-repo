// Copyright 2020-2023 Sophos Limited. All rights reserved.

// Class
#include "PolicyProcessor.h"

// Package
#include "Logger.h"

// Plugin
#include "common/ApplicationPaths.h"
#include "common/StringUtils.h"
#include "pluginimpl/ObfuscationImpl/Obfuscate.h"
#include "unixsocket/processControllerSocket/ProcessControllerClient.h"

// Product
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"

// Third party
#include <nlohmann/json.hpp>

// Std C
#include <sys/stat.h>

// Std C++
#include <fstream>

using json = nlohmann::json;

namespace
{
    constexpr int MAX_RETIRES_NOTIFY_SOAPD_ON_CONFIG_CHANGE = 10;

    bool boolFromString(const std::string& s)
    {
        if (s == "true")
        {
            return true;
        }
        if (s == "false")
        {
            return false;
        }
        std::ostringstream ost;
        ost << "Unable to convert " << s << " to boolean";
        throw Plugin::InvalidPolicyException(ost.str());
    }

    bool boolFromElement(const Common::XmlUtilities::Attributes& attribute, bool defaultValue)
    {
        if (attribute.empty())
        {
            return defaultValue;
        }
        return boolFromString(attribute.contents());
    }

    json readConfigFromFile(const std::string& filepath)
    {
        auto* fs = Common::FileSystem::fileSystem();

        try
        {
            auto contents = fs->readFile(filepath);
            return json::parse(contents);
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            // Could be the file doesn't exist
            // other errors will be reported when we try to save it.
            return {};
        }
        catch (const json::exception& ex)
        {
            LOGWARN("Failed to parse " << filepath << ": " << ex.what());
            return {};
        }
    }

    bool writeConfigToFile(const std::string& filepath, const json& configJson, const std::string& configName)
    {
        // convert to string
        auto config = configJson.dump();

        try
        {
            auto* fs = Common::FileSystem::fileSystem();
            auto tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
            fs->writeFileAtomically(filepath, config, tempDir, 0640);
            return true;
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(
                "Failed to write " << configName << ", Sophos On Access Process will use the default settings "
                                   << e.what());
            return false;
        }
    }

    using timepoint_t = Plugin::PolicyProcessor::timepoint_t;
    using seconds_t = Plugin::PolicyProcessor::seconds_t;

    timepoint_t getNow()
    {
        return Plugin::PolicyProcessor::clock_t::now();
    }
} // namespace

namespace Plugin
{
    namespace
    {
        std::string getNonChrootCustomerIdPath()
        {
            auto pluginInstall = getPluginInstall();
            return pluginInstall + "/var/customer_id.txt";
        }

        std::string getSoapControlSocketPath()
        {
            auto pluginInstall = getPluginInstall();
            return pluginInstall + "/var/soapd_controller";
        }

        std::string getSoapConfigPath()
        {
            auto pluginInstall = getPluginInstall();
            return pluginInstall + "/var/on_access_policy.json";
        }

        std::string getSoapFlagConfigPath()
        {
            auto pluginInstall = getPluginInstall();
            return pluginInstall + "/var/oa_flag.json";
        }
    } // namespace

    PolicyProcessor::PolicyProcessor(IStoppableSleeperSharedPtr stoppableSleeper) :
        m_sleeper(std::move(stoppableSleeper))
    {
        auto* fs = Common::FileSystem::fileSystem();
        auto dest = getNonChrootCustomerIdPath();

        try
        {
            m_customerId = fs->readFile(dest);
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            // Will happen the first time - so can't report it
            m_customerId = "";
        }
        loadSusiSettings();
    }

    /**
     * The only thing we are interested in from the ALC policy is getting the customer ID
     * which is required by SUSI inside ThreatDetector
     * @param policy
     */
    void PolicyProcessor::processAlcPolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
        resetTemporaryMarkerBooleans();

        auto oldCustomerId = m_customerId;
        m_customerId = getCustomerId(policy);

        if (!m_gotFirstAlcPolicy)
        {
            LOGINFO("ALC policy received for the first time.");
            m_gotFirstAlcPolicy = true;
        }

        if (m_customerId.empty())
        {
            LOGWARN("Failed to find customer ID from ALC policy");
            return;
        }

        if (m_customerId == oldCustomerId)
        {
            // customer ID not changed
            return;
        }

        // write customer ID to a file
        auto* fs = Common::FileSystem::fileSystem();
        auto dest = getNonChrootCustomerIdPath();
        fs->writeFile(dest, m_customerId);
        ::chmod(dest.c_str(), 0640);

        // write a copy into the chroot
        auto pluginInstall = getPluginInstall();
        auto chroot = pluginInstall + "/chroot";
        dest = chroot + dest;
        fs->writeFile(dest, m_customerId);
        ::chmod(dest.c_str(), 0640);

        reloadThreatDetectorConfiguration_ = true; // Only reload config for sophos_threat_detector if it changes
    }

    bool PolicyProcessor::getSXL4LookupsEnabled() const
    {
        return m_threatDetectorSettings.isSxlLookupEnabled();
    }

    std::string PolicyProcessor::getCustomerId(const Common::XmlUtilities::AttributesMap& policy)
    {
        auto primaryLocation = policy.lookup("AUConfigurations/AUConfig/primary_location/server");
        if (primaryLocation.empty())
        {
            return "";
        }
        std::string username{ primaryLocation.value("UserName") };
        std::string password{ primaryLocation.value("UserPassword") };
        std::string algorithm{ primaryLocation.value("Algorithm", "Clear") };

        // we check that username and password are not empty mainly for fuzzing purposes as in
        // product we never expect central to send us a policy with empty credentials
        if (password.empty())
        {
            throw std::invalid_argument("Invalid policy: Password is empty ");
        }
        if (username.empty())
        {
            throw std::invalid_argument("Invalid policy: Username is empty ");
        }

        bool unhashed = true;
        std::string cred;

        if (algorithm == "AES256")
        {
            password = Common::ObfuscationImpl::SECDeobfuscate(password);
        }
        else if (algorithm != "Clear")
        {
            throw std::invalid_argument("Invalid policy: Unknown obfuscation algorithm");
        }

        if (username.size() == 32 && username == password)
        {
            unhashed = false;
            cred = username;
        }

        if (unhashed)
        {
            cred = Common::SslImpl::calculateDigest(Common::SslImpl::Digest::md5, username + ':' + password);
        }
        return Common::SslImpl::calculateDigest(Common::SslImpl::Digest::md5, cred); // always do the second hash
    }

    void PolicyProcessor::markOnAccessReloadPending()
    {
        // Try MAX_RETIRES_NOTIFY_SOAPD_ON_CONFIG_CHANGE times to send reload to soapd - if it doesn't receive the
        // message, it will probably read the new config file when it starts up
        m_pendingOnAccessProcessReload = MAX_RETIRES_NOTIFY_SOAPD_ON_CONFIG_CHANGE;
    }

    void PolicyProcessor::notifyOnAccessProcessIfRequired()
    {
        if (m_pendingOnAccessProcessReload > 0)
        {
            m_pendingOnAccessProcessReload--; // Only retry a finite amount of times
            unixsocket::ProcessControllerClientSocket processController(getSoapControlSocketPath(), m_sleeper, 0);
            if (processController.isConnected())
            {
                // Successfully connected to soapd
                scan_messages::ProcessControlSerialiser processControlRequest(scan_messages::E_COMMAND_TYPE::E_RELOAD);
                processController.sendProcessControlRequest(processControlRequest);
                m_pendingOnAccessProcessReload = 0;
            }
        }
    }

    void PolicyProcessor::processOnAccessSettingsFromSAVpolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
#ifndef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
        return;
#endif
        LOGINFO("Processing On Access Scanning settings from SAV policy");
        const auto originalConfig = readOnAccessConfig();
        auto config = originalConfig;

        auto exclusionList = extractListFromXML(policy, "config/onAccessScan/linuxExclusions/filePathSet/filePath");
        json exclusions(exclusionList);
        config["exclusions"] = exclusions;

        auto oaPuaDetectionEnabled = boolFromElement(policy.lookup("config/onAccessScan/scanBehaviour/pua"), true);
        config["detectPUAs"] = oaPuaDetectionEnabled;

        if (originalConfig != config)
        {
            writeOnAccessConfig(config);
        }
        else
        {
            LOGDEBUG("On Access settings from SAV policy not changed");
        }
    }

    void PolicyProcessor::setOnAccessConfiguredTelemetry(bool enabled)
    {
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        Common::Telemetry::TelemetryObject onAccessEnabledTelemetry;
        onAccessEnabledTelemetry.set(Common::Telemetry::TelemetryValue(enabled));
        telemetry.set("onAccessConfigured", onAccessEnabledTelemetry, true);
    }

    void PolicyProcessor::processSavPolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
        resetTemporaryMarkerBooleans();

        processOnAccessSettingsFromSAVpolicy(policy);

        // Need to save SUSI settings first time to ensure defaults are saved to disk
        bool needToSave = !m_gotFirstSavPolicy;

        // Lookup enabled
#ifndef USE_SXL_ENABLE_FROM_CORC_POLICY
        bool oldLookupEnabled = m_threatDetectorSettings.isSxlLookupEnabled();
        bool lookupEnabledInPolicy = isLookupEnabledFromSavPolicy(policy);
        if (oldLookupEnabled != lookupEnabledInPolicy)
        {
            // Only need to change object or restart TD if the setting has changed
            // Need to save if we got the first policy since restarting AV plugin as well
            m_threatDetectorSettings.setSxlLookupEnabled(lookupEnabledInPolicy);
            m_restartThreatDetector = true;
            needToSave = true;
        }
#endif

        // PUA Approved list
        auto oldPuaApprovedList = m_threatDetectorSettings.copyPuaApprovedList();
        auto puaApprovedListFromPolicy = extractListFromXML(policy, "config/approvedList/puaName");
        if (oldPuaApprovedList != puaApprovedListFromPolicy)
        {
            m_threatDetectorSettings.setPuaApprovedList(std::move(puaApprovedListFromPolicy));
            needToSave = true;
        }

        if (needToSave)
        {
            saveSusiSettings("SAV");
        }

        if (!m_gotFirstSavPolicy)
        {
            LOGINFO("SAV policy received for the first time.");
            m_gotFirstSavPolicy = true;
        }
    }

    bool PolicyProcessor::isLookupEnabledFromSavPolicy(const AttributesMap& policy)
    {
        auto contents = policy.lookup("config/detectionFeedback/sendData").contents();
        if (contents == "true" || contents == "false")
        {
            return contents == "true";
        }
        // Default to false if we can't read or understand the sendData value
        return common::ThreatDetector::SXL_DEFAULT;
    }

    bool PolicyProcessor::isLookupEnabledFromCorcPolicy(const PolicyProcessor::AttributesMap& policy)
    {
        auto contents = policy.lookup("policy/detectionFeedback/sendData").contents();
        if (contents == "true" || contents == "false")
        {
            return contents == "true";
        }
        // Default to false if we can't read or understand the sendData value
        return common::ThreatDetector::SXL_DEFAULT;
    }

    std::vector<std::string> PolicyProcessor::extractListFromXML(
        const Common::XmlUtilities::AttributesMap& policy,
        const std::string& entityFullPath)
    {
        std::vector<std::string> results;
        auto attrs = policy.lookupMultiple(entityFullPath);
        results.reserve(attrs.size());
        for (const auto& attr : attrs)
        {
            results.emplace_back(attr.contents());
        }
        return results;
    }

    void PolicyProcessor::processFlagSettings(const std::string& flagsJson)
    {
        LOGDEBUG("Processing FLAGS settings");
        try
        {
            auto j = json::parse(flagsJson);
            processOnAccessFlagSettings(j);
            processSafeStoreFlagSettings(j);
        }
        catch (const json::parse_error& e)
        {
            LOGWARN("Failed to parse FLAGS policy due to parse error, reason: " << e.what());
        }
    }

    void PolicyProcessor::processOnAccessFlagSettings(const json& flagsJson)
    {
        bool oaEnabled = false;

        if (flagsJson.find(OA_FLAG) != flagsJson.end())
        {
            if (flagsJson[OA_FLAG] == true)
            {
                LOGINFO("On-access is enabled in the FLAGS policy, assuming on-access policy settings");
                oaEnabled = true;
            }
            else
            {
                LOGINFO("On-access is disabled in the FLAGS policy, overriding on-access policy settings");
            }
        }
        else
        {
            LOGINFO("No on-access flag found, overriding on-access policy settings");
        }

        auto path = getSoapFlagConfigPath();
        auto orignal_config = readConfigFromFile(path);

        json oaConfig;
        oaConfig["oa_enabled"] = oaEnabled;

        if (orignal_config != oaConfig)
        {
            if (writeConfigToFile(path, oaConfig, "Flag Config"))
            {
                markOnAccessReloadPending();
            }
        }
        else
        {
            LOGINFO("On Access setting from FLAGS policy not changed");
        }
    }

    void PolicyProcessor::processSafeStoreFlagSettings(const nlohmann::json& flagsJson)
    {
        bool ssEnabled = flagsJson.value(SS_FLAG, false);
        if (ssEnabled)
        {
            LOGINFO("SafeStore flag set. Setting SafeStore to enabled.");
        }
        else
        {
            LOGINFO("SafeStore flag not set. Setting SafeStore to disabled.");
        }
        m_safeStoreEnabled = ssEnabled;

        if (m_safeStoreEnabled)
        {
            m_safeStoreQuarantineMl = flagsJson.value(SS_ML_FLAG, false);
            if (m_safeStoreQuarantineMl)
            {
                LOGDEBUG("SafeStore Quarantine ML flag set. SafeStore will quarantine ML detections.");
            }
            else
            {
                LOGDEBUG("SafeStore Quarantine ML flag not set. SafeStore will not quarantine ML detections.");
            }
        }
    }

    bool PolicyProcessor::isSafeStoreEnabled() const
    {
        return m_safeStoreEnabled;
    }

    bool PolicyProcessor::shouldSafeStoreQuarantineMl() const
    {
        return m_safeStoreQuarantineMl;
    }

    PolicyType PolicyProcessor::determinePolicyType(
        const Common::XmlUtilities::AttributesMap& policy,
        const std::string& appId)
    {
        if (appId == "ALC")
        {
            return PolicyType::ALC;
        }
        else if (appId == "CORE")
        {
            return PolicyType::CORE;
        }
        else if (appId == "CORC")
        {
            return PolicyType::CORC;
        }
        else if (appId == "SAV")
        {
            auto policyType = policy.lookup("config/csc:Comp").value("policyType", "unknown");
            if (policyType == "2")
            {
                return PolicyType::SAV;
            }
        }
        return PolicyType::UNKNOWN;
    }

    void PolicyProcessor::processCOREpolicy(const AttributesMap& policy)
    {
        resetTemporaryMarkerBooleans();

        processOnAccessSettingsFromCOREpolicy(policy);
        processSusiSettingsFromCOREpolicy(policy);
    }

    void PolicyProcessor::processOnAccessSettingsFromCOREpolicy(const AttributesMap& policy)
    {
        const auto on_access_element = policy.lookupMultiple("policy/onAccessScan");
        if (on_access_element.size() != 1)
        {
            LOGDEBUG("CORE policy lacks onAccessScan element");
            return;
        }

        LOGINFO("Processing On Access Scanning settings from CORE policy");
        const auto on_access_enabled = boolFromElement(policy.lookup("policy/onAccessScan/enabled"), false);
        const auto excludeRemoteFiles = boolFromElement(
            policy.lookup("policy/onAccessScan/exclusions/excludeRemoteFiles"), false);
        const auto onOpen = boolFromElement(
            policy.lookup("policy/onAccessScan/onRead"), true);
        const auto onClose = boolFromElement(
            policy.lookup("policy/onAccessScan/onWrite"), true);

        const auto originalConfig = readOnAccessConfig();
        auto config = originalConfig;
        config["excludeRemoteFiles"] = excludeRemoteFiles;
        config["enabled"] = on_access_enabled;
        config["onOpen"] = onOpen;
        config["onClose"] = onClose;

#ifndef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
        // Assuming the Linux exclusions are put into the generic location in the XML
        auto exclusionList = extractListFromXML(policy, "policy/onAccessScan/exclusions/filePathSet/filePath");
        json exclusions(exclusionList);
        config["exclusions"] = exclusions;
#endif

        if (originalConfig != config)
        {
            if(on_access_enabled)
            {
                LOGINFO("On Access is enabled in CORE policy");
            }
            else
            {
                LOGINFO("On Access is disabled in CORE policy");
            }
            writeOnAccessConfig(config);
            setOnAccessConfiguredTelemetry(on_access_enabled);
        }
        else
        {
            LOGINFO("On Access settings from CORE policy not changed");
        }
    }

    void PolicyProcessor::processSusiSettingsFromCOREpolicy(const PolicyProcessor::AttributesMap& policy)
    {
        const auto on_access_element = policy.lookupMultiple("policy/coreFeatures");
        if (on_access_element.size() != 1)
        {
            LOGDEBUG("CORE policy lacks coreFeatures element");
            return;
        }

        bool changed = !m_susiSettingsWritten; // Always write the first time

        const auto machineLearningEnabled = boolFromString(policy.lookup("policy/coreFeatures/machineLearningEnabled").contents());
        const auto existingSetting = m_threatDetectorSettings.isMachineLearningEnabled();

        Common::Telemetry::TelemetryObject mlSettingTelemetry;
        mlSettingTelemetry.set(Common::Telemetry::TelemetryValue(machineLearningEnabled));
        Common::Telemetry::TelemetryHelper::getInstance().set("ml-scanning-enabled", mlSettingTelemetry, true);

        if (machineLearningEnabled != existingSetting)
        {
            m_threatDetectorSettings.setMachineLearningEnabled(machineLearningEnabled);
            LOGINFO("Machine Learning detections "<< (machineLearningEnabled?"enabled":"disabled") << " in CORE policy");
            changed = true;
        }

        if (changed)
        {
            saveSusiSettings("CORE");
        }
        assert(m_susiSettingsWritten);
    }

    nlohmann::json PolicyProcessor::readOnAccessConfig()
    {
        return readConfigFromFile(getSoapConfigPath());
    }

    void PolicyProcessor::writeOnAccessConfig(const json& configJson)
    {
        if (writeConfigToFile(getSoapConfigPath(), configJson, "On Access Config"))
        {
            markOnAccessReloadPending();
        }
    }

    PolicyProcessor::timepoint_t PolicyProcessor::timeout() const
    {
        return timeout(getNow());
    }

    timepoint_t PolicyProcessor::timeout(timepoint_t now) const
    {
        if (m_pendingOnAccessProcessReload == MAX_RETIRES_NOTIFY_SOAPD_ON_CONFIG_CHANGE)
        {
            return now + std::chrono::milliseconds{ 10 };
        }
        else if (m_pendingOnAccessProcessReload > 0)
        {
            return now + seconds_t{ 1 };
        }
        else
        {
            // Not waiting to send anything to soapd
            static constexpr seconds_t MAX_TIMEOUT{ 9999999 };
            return now + MAX_TIMEOUT;
        }
    }

    void PolicyProcessor::resetTemporaryMarkerBooleans()
    {
        m_restartThreatDetector = false;
        reloadThreatDetectorConfiguration_ = false;
    }

    void PolicyProcessor::processCorcPolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
        resetTemporaryMarkerBooleans();

        bool firstPolicy = !m_gotFirstCorcPolicy;
        if (!m_gotFirstCorcPolicy)
        {
            LOGINFO("CORC policy received for the first time");
            m_gotFirstCorcPolicy = true;
        }
        bool changed = firstPolicy;

        auto allowListFromPolicy = policy.lookupMultiple("policy/whitelist/item");
        std::vector<std::string> sha256AllowList;
        std::vector<std::string> pathAllowList;

        for (const auto& allowedItem : allowListFromPolicy)
        {
            if (allowedItem.value("type") == "sha256" && !allowedItem.contents().empty())
            {
                LOGDEBUG("Added SHA256 to allow list: " << allowedItem.contents());
                sha256AllowList.emplace_back(allowedItem.contents());
            }
            if (allowedItem.value("type") == "path" && !allowedItem.contents().empty())
            {
                if (allowedItem.contents().front() == '*' || allowedItem.contents().front() == '/')
                {
                    LOGDEBUG("Added path to allow list: " << allowedItem.contents());
                    pathAllowList.emplace_back(allowedItem.contents());
                }
                else
                {
                    LOGWARN("Ignoring invalid allow list path item: " << allowedItem.contents() << ", as it doesnt start with * or /");
                }
            }
        }

        // Lookup enabled
#ifdef USE_SXL_ENABLE_FROM_CORC_POLICY
        bool oldLookupEnabled = m_threatDetectorSettings.isSxlLookupEnabled();
        bool lookupEnabledInPolicy = isLookupEnabledFromCorcPolicy(policy);
        if (oldLookupEnabled != lookupEnabledInPolicy)
        {
            // Only need to change object or restart TD if the setting has changed
            // Need to save if we got the first policy since restarting AV plugin as well
            m_threatDetectorSettings.setSxlLookupEnabled(lookupEnabledInPolicy);
            m_restartThreatDetector = true;
            changed = true;
        }
#endif

        auto oldSxlUrl = m_threatDetectorSettings.getSxlUrl();
        std::string sxlUrl;
        auto sxlUrls = policy.lookupMultiple("policy/intelix/lookupUrl");
        if (sxlUrls.empty())
        {
            LOGWARN("CORC policy doesn't contain lookupUrl: Disabling Global Reputation");
        }
        else if (sxlUrls.size() > 1)
        {
            LOGWARN("CORC policy contains multiple lookupUrl: Disabling Global Reputation");
        }
        else
        {
            sxlUrl = sxlUrls[0].contents();
            if (!isLookupUrlValid(sxlUrl))
            {
                LOGERROR("Invalid lookupUrl: " << sxlUrl);
                sxlUrl = "";
            }
            else
            {
                // Policy contains:
                // <lookupUrl>https://4.sophosxl.net</lookupUrl>
                // but SUSI wants:
                // "url": "https://4.sophosxl.net/lookup",
                if (!Common::UtilityImpl::StringUtils::endswith(sxlUrl, "/lookup"))
                {
                    sxlUrl = sxlUrl + "/lookup";
                }
            }
            LOGDEBUG("SXL URL: " << sxlUrl);
        }
        if (oldSxlUrl != sxlUrl)
        {
            // Only need to change object or restart TD if the setting has changed
            m_threatDetectorSettings.setSxlUrl(sxlUrl);
            if (lookupEnabledInPolicy)
            {
                // Need to restart TD in order for new URL to be used.
                // But we only need to restart if SXL is enabled!
                m_restartThreatDetector = true;
            }
            changed = true;
        }

        auto oldSha256AllowList = m_threatDetectorSettings.copyAllowListSha256();
        auto oldPathAllowList = m_threatDetectorSettings.copyAllowListPath();
        if (oldSha256AllowList != sha256AllowList)
        {
            changed = true;
        }
        else if (oldPathAllowList != pathAllowList)
        {
            changed = true;
        }

        if (changed)
        {
            m_threatDetectorSettings.setAllowListSha256(std::move(sha256AllowList));
            m_threatDetectorSettings.setAllowListPath(std::move(pathAllowList));
            saveSusiSettings("CORC");
        }
        else
        {
            LOGDEBUG("SUSI settings from CORC policy not changed");
        }
    }

    bool PolicyProcessor::isLookupUrlValid(const std::string& url)
    {
        return common::ThreatDetector::SusiSettings::isSxlUrlValid(url);
    }

    void PolicyProcessor::saveSusiSettings(const std::string& policyName)
    {
        auto dest = Plugin::getSusiStartupSettingsPath();
        m_threatDetectorSettings.saveSettings(dest, 0640);

        // Also, write a copy to chroot
        dest = Plugin::getPluginInstall() + "/chroot" + dest;
        m_threatDetectorSettings.saveSettings(dest, 0640);

        // Make SUSI reload config
        reloadThreatDetectorConfiguration_ = true;
        LOGINFO("Saved Threat Detector SUSI settings for " << policyName << " policy");
        m_susiSettingsWritten = true;
    }

    void PolicyProcessor::loadSusiSettings()
    {
        auto src = Plugin::getSusiStartupSettingsPath();
        std::ignore = m_threatDetectorSettings.load(src);
    }
} // namespace Plugin
