// Copyright 2020-2022, Sophos Limited.  All rights reserved.

// Class
#include "PolicyProcessor.h"
// Package
#include "Logger.h"
#include "StringUtils.h"
// Plugin
#include "unixsocket/processControllerSocket/ProcessControllerClient.h"
// Product
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <common/StringUtils.h>
#include <pluginimpl/ObfuscationImpl/Obfuscate.h>
// Third party
#include <thirdparty/nlohmann-json/json.hpp>
// Std C
#include <sys/stat.h>

using json = nlohmann::json;

namespace Plugin
{
    namespace
    {
        std::string getPluginInstall()
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            return appConfig.getData("PLUGIN_INSTALL");
        }

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
            auto  pluginInstall = getPluginInstall();
            return  pluginInstall + "/var/soapd_controller";
        }

        std::string getSoapConfigPath()
        {
            auto  pluginInstall = getPluginInstall();
            return  pluginInstall + "/var/soapd_config.json";
        }

        std::string getSoapFlagConfigPath()
        {
            auto  pluginInstall = getPluginInstall();
            return  pluginInstall + "/var/oa_flag.json";
        }

        std::string getSafestoreFlagConfigPath()
        {
            auto  pluginInstall = getPluginInstall();
            return  pluginInstall + "/var/ss_flag.json";
        }
    }

    PolicyProcessor::PolicyProcessor()
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

    bool PolicyProcessor::processAlcPolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
        auto oldCustomerId = m_customerId;
        m_customerId = getCustomerId(policy);
        if (m_customerId.empty())
        {
            LOGWARN("Failed to find customer ID from ALC policy");
            return false;
        }

        if (m_customerId == oldCustomerId)
        {
            // customer ID not changed
            return false;
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

        return true; // Only restart sophos_threat_detector if it changes
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
        unixsocket::ProcessControllerClientSocket processController(getSoapControlSocketPath());
        scan_messages::ProcessControlSerialiser processControlRequest(requestType);
        processController.sendProcessControlRequest(processControlRequest);
    }

    void PolicyProcessor::processOnAccessPolicy(const Common::XmlUtilities::AttributesMap& policy)
    {
        LOGINFO("Processing On Access Scanning settings");

        auto excludeRemoteFiles = policy.lookup("config/onAccessScan/linuxExclusions/excludeRemoteFiles").contents();
        auto exclusionList = extractListFromXML(policy, "config/onAccessScan/linuxExclusions/filePathSet/filePath");
        //TODO: LINUXDAR-5352 update the xml path, this setting will be off by default
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
            LOGERROR("Failed to write On Access Config, Sophos On Access Process will use the default settings " << e.what());
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

    bool PolicyProcessor::processSavPolicy(const Common::XmlUtilities::AttributesMap& policy, bool isSAVPolicyAlreadyProcessed)
    {
        processOnAccessPolicy(policy);

        bool oldLookupEnabled = m_lookupEnabled;
        m_lookupEnabled = isLookupEnabled(policy);
        if (isSAVPolicyAlreadyProcessed && m_lookupEnabled == oldLookupEnabled)
        {
            // Only restart sophos_threat_detector if it is the first policy or it has changed

            return false;
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

        return true;
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

    std::vector<std::string> PolicyProcessor::extractListFromXML(const Common::XmlUtilities::AttributesMap& policy, const std::string& entityFullPath)
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
                LOGINFO("On-access is enabled in the FLAGS policy, notifying soapd to disable on-access policy "
                        "override");
                oaEnabled = true;
            }
            else
            {
                LOGINFO("On-access is disabled in the FLAGS policy, notifying soapd to enable on-access policy "
                        "override");
            }
        }
        else
        {
            LOGINFO("No on-access flag found assuming policy settings");
            oaEnabled = true;
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
            LOGINFO("Safestore flag set. Setting Safestore to enabled.");
        }
        else
        {
            LOGINFO("Safestore flag not set. Setting Safestore to disabled.");
        }

        try
        {
            json ssConfig;
            ssConfig["ss_enabled"] = ssEnabled;

            auto* fs = Common::FileSystem::fileSystem();
            auto tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
            fs->writeFileAtomically(getSafestoreFlagConfigPath(), ssConfig.dump(), tempDir, 0640);

            // TODO: LINUXDAR-5632 -- Need to notify TD + SS to check new config here, like with OA above
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(
                "Failed to write Flag Config, Sophos SafeStore Process will use the default settings (safestore "
                "disabled)"
                << e.what());
        }
    }
}
