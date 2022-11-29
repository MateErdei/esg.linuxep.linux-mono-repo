// Copyright 2020-2022, Sophos Limited.  All rights reserved.

// Class
#include "PolicyProcessor.h"
// Package
#include "Logger.h"
#include "StringUtils.h"
// Plugin
#include "common/ApplicationPaths.h"
#include "unixsocket/processControllerSocket/ProcessControllerClient.h"
// Product
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <common/StringUtils.h>
#include <pluginimpl/ObfuscationImpl/Obfuscate.h>
// Std C++
#include <fstream>
// Std C
#include <sys/stat.h>

using json = nlohmann::json;

namespace
{
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
            return json{};
        }
    }
}

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
            return pluginInstall + "/var/soapd_config.json";
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
    }

    void PolicyProcessor::processAlcPolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
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
            m_restartThreatDetector = false;
            return;
        }

        if (m_customerId == oldCustomerId)
        {
            // customer ID not changed
            m_restartThreatDetector = false;
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

        m_restartThreatDetector = true; // Only restart sophos_threat_detector if it changes
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
        std::string username { primaryLocation.value("UserName") };
        std::string password { primaryLocation.value("UserPassword") };
        std::string algorithm { primaryLocation.value("Algorithm", "Clear") };

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
            cred = common::md5_hash(username + ':' + password);
        }
        return common::md5_hash(cred); // always do the second hash
    }

    void PolicyProcessor::notifyOnAccessProcess(scan_messages::E_COMMAND_TYPE requestType)
    {
        unixsocket::ProcessControllerClientSocket processController(getSoapControlSocketPath(), m_sleeper);
        scan_messages::ProcessControlSerialiser processControlRequest(requestType);
        processController.sendProcessControlRequest(processControlRequest);
    }

    void PolicyProcessor::processOnAccessSettingsFromSAVpolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
#ifndef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
        return;
#endif
        LOGINFO("Processing On Access Scanning settings from SAV policy");
        json config = readOnAccessConfig();

        auto exclusionList = extractListFromXML(policy, "config/onAccessScan/linuxExclusions/filePathSet/filePath");
        json exclusions(exclusionList);
        config["exclusions"] = exclusions;
        writeOnAccessConfig(config);
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
        processOnAccessSettingsFromSAVpolicy(policy);

        bool oldLookupEnabled = m_lookupEnabled;
        m_lookupEnabled = isLookupEnabled(policy);

        if (m_gotFirstSavPolicy && m_threatDetectorSettings.isSxlLookupEnabled() == oldLookupEnabled)
        {
            // Don't restart Threat Detector if config has not changed, and it's not the first policy
            m_restartThreatDetector = false;
            return;
        }

        if (!m_gotFirstSavPolicy)
        {
            LOGINFO("SAV policy received for the first time.");
            m_gotFirstSavPolicy = true;
        }

        saveSusiSettings();
        m_restartThreatDetector = true;
    }

    bool PolicyProcessor::isLookupEnabled(const Common::XmlUtilities::AttributesMap& policy)
    {
        auto contents = policy.lookup("config/detectionFeedback/sendData").contents();
        if (contents == "true" || contents == "false")
        {
            return contents == "true";
        }
        // Default to true if we can't read or understand the sendData value
        return true;
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
            nlohmann::json j = nlohmann::json::parse(flagsJson);
            processOnAccessFlagSettings(j);
            processSafeStoreFlagSettings(j);
        }
        catch (const json::parse_error& e)
        {
            LOGWARN("Failed to parse FLAGS policy due to parse error, reason: " << e.what());
        }
    }

    void PolicyProcessor::processOnAccessFlagSettings(const nlohmann::json& flagsJson)
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

        json oaConfig;
        oaConfig["oa_enabled"] = oaEnabled;

        try
        {
            auto* fs = Common::FileSystem::fileSystem();
            auto tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
            fs->writeFileAtomically(getSoapFlagConfigPath(), oaConfig.dump(), tempDir, 0640);

            notifyOnAccessProcess(scan_messages::E_COMMAND_TYPE::E_RELOAD);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(
                "Failed to write Flag Config, Sophos On Access Process will use the default settings (on-access "
                "disabled)"
                << e.what());
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
    }

    bool PolicyProcessor::isSafeStoreEnabled() const
    {
        return m_safeStoreEnabled;
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
        processOnAccessSettingsFromCOREpolicy(policy);
    }

    void PolicyProcessor::processOnAccessSettingsFromCOREpolicy(const AttributesMap& policy)
    {
        LOGINFO("Processing On Access Scanning settings from CORE policy");
        const auto on_access_enabled = boolFromString(policy.lookup("policy/onAccessScan/enabled").contents());
        const auto excludeRemoteFiles = boolFromString(
            policy.lookup("policy/onAccessScan/exclusions/excludeRemoteFiles").contents());

        json config = readOnAccessConfig();
        config["excludeRemoteFiles"] = excludeRemoteFiles;
        config["enabled"] = on_access_enabled;

#ifndef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
        // Assuming the Linux exclusions are put into the generic location in the XML
        auto exclusionList = extractListFromXML(policy, "policy/onAccessScan/exclusions/filePathSet/filePath");
        json exclusions(exclusionList);
        config["exclusions"] = exclusions;
#endif
        writeOnAccessConfig(config);
        setOnAccessConfiguredTelemetry(on_access_enabled);
    }

    nlohmann::json PolicyProcessor::readOnAccessConfig()
    {
        return readConfigFromFile(getSoapConfigPath());
    }

    void PolicyProcessor::writeOnAccessConfig(const json& configJson)
    {
        // convert to string
        auto config = configJson.dump();

        try
        {
            auto* fs = Common::FileSystem::fileSystem();
            auto tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
            fs->writeFileAtomically(getSoapConfigPath(), config, tempDir, 0640);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Failed to write On Access Config, Sophos On Access Process will use the default settings " << e.what());
            return;
        }

        notifyOnAccessProcess(scan_messages::E_COMMAND_TYPE::E_RELOAD);
    }
}

    void PolicyProcessor::processCorcPolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
        if (!m_gotFirstCorcPolicy)
        {
            LOGINFO("CORC policy received for the first time");
            m_gotFirstCorcPolicy = true;
        }

        auto allowListFromPolicy = policy.lookupMultiple("policy/whitelist/item");
        std::vector<std::string> sha256AllowList;
        for (const auto& allowedItem : allowListFromPolicy)
        {
            if (allowedItem.value("type") == "sha256" && !allowedItem.contents().empty())
            {
                LOGDEBUG("Added SHA256 to allow list: " << allowedItem.contents());
                sha256AllowList.emplace_back(allowedItem.contents());
            }
        }
        m_threatDetectorSettings.setAllowList(std::move(sha256AllowList));
        saveSusiSettings();
    }

    void PolicyProcessor::saveSusiSettings()
    {
        auto dest = Plugin::getSusiStartupSettingsPath();
        m_threatDetectorSettings.saveSettings(dest, 0640);

        // Also, write a copy to chroot
        dest = Plugin::getPluginInstall() + "/chroot" + dest;
        m_threatDetectorSettings.saveSettings(dest, 0640);
    }
} // namespace Plugin
