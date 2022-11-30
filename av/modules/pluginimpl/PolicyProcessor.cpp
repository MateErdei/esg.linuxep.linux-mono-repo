// Copyright 2020-2022 Sophos Limited. All rights reserved.

// Class
#include "PolicyProcessor.h"

// Package
#include "Logger.h"
#include "StringUtils.h"

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

// Std C
#include <sys/stat.h>

// Third party
#include <thirdparty/nlohmann-json/json.hpp>

using json = nlohmann::json;

namespace Plugin
{
    namespace
    {
        std::string getNonChrootCustomerIdPath()
        {
            auto pluginInstall = getPluginInstall();
            return pluginInstall + "/var/customer_id.txt";
        }

        std::string getSusiStartupSettingsPath()
        {
            auto pluginInstall = getPluginInstall();
            return pluginInstall + "/var/susi_startup_settings.json";
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
        return m_lookupEnabled;
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

    void PolicyProcessor::processOnAccessPolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
        LOGINFO("Processing On Access Scanning settings");

        auto excludeRemoteFiles = policy.lookup("config/onAccessScan/linuxExclusions/excludeRemoteFiles").contents();
        auto exclusionList = extractListFromXML(policy, "config/onAccessScan/linuxExclusions/filePathSet/filePath");
        // TODO: LINUXDAR-5352 update the xml path, this setting will be off by default
        auto enabled = policy.lookup("config/onAccessScan/enabled").contents();
        auto config = pluginimpl::generateOnAccessConfig(enabled, exclusionList, excludeRemoteFiles);

        try
        {
            auto* fs = Common::FileSystem::fileSystem();
            auto tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
            fs->writeFileAtomically(getSoapConfigPath(), config, tempDir, 0640);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(
                "Failed to write On Access Config, Sophos On Access Process will use the default settings "
                << e.what());
            return;
        }

        notifyOnAccessProcess(scan_messages::E_COMMAND_TYPE::E_RELOAD);

        setOnAccessConfiguredTelemetry(enabled == "true");
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
        processOnAccessPolicy(policy);

        bool oldLookupEnabled = m_lookupEnabled;
        m_lookupEnabled = isLookupEnabled(policy);

        if (m_gotFirstSavPolicy && m_lookupEnabled == oldLookupEnabled)
        {
            // Dont restart Threat Detector if its not changed and its not the first policy
            m_restartThreatDetector = false;
            return;
        }

        if (!m_gotFirstSavPolicy)
        {
            LOGINFO("SAV policy received for the first time.");
            m_gotFirstSavPolicy = true;
        }

        json susiStartupSettings;
        susiStartupSettings["enableSxlLookup"] = m_lookupEnabled;

        try
        {
            auto* fs = Common::FileSystem::fileSystem();

            // Write settings to file
            auto dest = getSusiStartupSettingsPath();
            fs->writeFile(dest, susiStartupSettings.dump());
            ::chmod(dest.c_str(), 0640);

            // Write a copy to chroot
            dest = Plugin::getPluginInstall() + "/chroot" + dest;
            fs->writeFile(dest, susiStartupSettings.dump());
            ::chmod(dest.c_str(), 0640);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(e.what() << ", setting default values for susi startup settings.");
        }

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
} // namespace Plugin
