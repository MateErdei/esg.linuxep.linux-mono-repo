// Copyright 2020-2023 Sophos Limited. All rights reserved.

// Class
#include "PluginCallback.h"
// Package
#include "Logger.h"
#include "PolicyProcessor.h"
// Component
#include "common/PluginUtils.h"
#include "common/PidLockFile.h"
#include "common/StringUtils.h"
// Plugin
#include "common/ApplicationPaths.h"
// Product
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/XmlUtilities/AttributesMap.h"
// Third party
#include <nlohmann/json.hpp>
// Std C++
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

namespace fs = sophos_filesystem;

using namespace std::chrono_literals;

namespace Plugin
{
    PluginCallback::PluginCallback(std::shared_ptr<TaskQueue> task)
    : m_task(std::move(task))
    , m_revID(std::string())
    {
        std::string noPolicySetStatus = generateSAVStatusXML();
        m_statusInfo = { noPolicySetStatus, noPolicySetStatus, "SAV" };

        // Initially configure On-Access off, until we get a policy
        Plugin::PolicyProcessor::setOnAccessConfiguredTelemetry(false);

        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string& /* policyXml */)
    {
        LOGERROR("This method should never be called.");
    }

    void PluginCallback::applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml)
    {
        LOGSUPPORT("Applying new policy with APPID: " << appId);
        m_task->push(Task { Task::TaskType::Policy, policyXml, appId });
    }

    void PluginCallback::queueAction(const std::string& actionXml )
    {
        LOGSUPPORT("Queueing action");
        m_task->push(Task{Task::TaskType::Action, actionXml });
    }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_task->pushStop();

        auto deadline = std::chrono::steady_clock::now() + 30s;
        auto nextLog = std::chrono::steady_clock::now() + 1s;

        while(isRunning() && std::chrono::steady_clock::now() < deadline)
        {
            if ( std::chrono::steady_clock::now() > nextLog )
            {
                nextLog = std::chrono::steady_clock::now() + 1s;
                LOGSUPPORT("Shutdown waiting for all processes to complete");
            }
            std::this_thread::sleep_for(50ms);
        }
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string& appId)
    {
        if (appId != "SAV")
        {
            LOGERROR("Received status request for unexpected appId: "<< appId);
            return { "", "", ""};
        }
        LOGSUPPORT("Received get status request for "<<appId);

        std::string status = generateSAVStatusXML();
        m_statusInfo = { status, status, "SAV" };
        return m_statusInfo;
    }

    void PluginCallback::setSXL4Lookups(bool sxl4Lookup)
    {
        m_lookupEnabled = sxl4Lookup;
    }

    void PluginCallback::setThreatHealth(E_HEALTH_STATUS threatStatus)
    {
        if (m_threatStatus != threatStatus)
        {
            LOGINFO("Threat health changed to " << threatHealthToString(threatStatus));
        }
        m_threatStatus = threatStatus;
    }

    long PluginCallback::getThreatHealth() const
    {
        return m_threatStatus;
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        telemetry.set("sxl4-lookup", m_lookupEnabled);
        telemetry.set("threatHealth", m_threatStatus);

        auto sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
        auto health = calculateHealth(sysCalls);
        telemetry.set("health", health);

        auto* fileSystem = Common::FileSystem::fileSystem();
        Telemetry telemetryCalculator{sysCalls, fileSystem};
        auto telemetryJson = telemetryCalculator.getTelemetry();

        // Reset threatHealth
        telemetry.set("threatHealth", m_threatStatus);
        return telemetryJson;
    }

    bool PluginCallback::shutdownFileValid(Common::FileSystem::IFileSystem* fileSystem) const
    {
        // Threat detector is expected to shut down periodically but should be restarted by watchdog after 10 seconds
        Path threatDetectorExpectedShutdownFile = common::getPluginInstallPath() / "chroot/var/threat_detector_expected_shutdown";
        bool valid = false;

        try
        {
            if (fileSystem->exists(threatDetectorExpectedShutdownFile))
            {
                std::time_t currentTime = std::time(nullptr);
                std::time_t lastModifiedTime = fileSystem->lastModifiedTime(threatDetectorExpectedShutdownFile);

                if (currentTime - lastModifiedTime < m_allowedShutdownTime)
                {
                    valid = true;
                }
            }
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGDEBUG("Error accessing threat detector expected shutdown file: " << threatDetectorExpectedShutdownFile << " due to: " << e.what() << " --- treating file as invalid");
        }

        return valid;
    }

    std::string PluginCallback::generateSAVStatusXML()
    {
        std::string result = Common::UtilityImpl::StringUtils::orderedStringReplace(
                R"sophos(<?xml version="1.0" encoding="utf-8"?>
<status xmlns="http://www.sophos.com/EE/EESavStatus">
  <csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="@@POLICY_COMPLIANCE@@" RevID="@@REV_ID@@" policyType="2"/>
  <upToDateState>1</upToDateState>
  <vdl-info>
    <virus-engine-version>N/A</virus-engine-version>
    <virus-data-version>N/A</virus-data-version>
    <idelist>
    </idelist>
    <ideChecksum>N/A</ideChecksum>
  </vdl-info>
  <on-access>@@ON_ACCESS_STATUS@@</on-access>
  <entity>
    <productId>SSPL-AV</productId>
    <product-version>@@PLUGIN_VERSION@@</product-version>
    <entityInfo>SSPL-AV</entityInfo>
  </entity>
</status>)sophos",{
                        {"@@POLICY_COMPLIANCE@@", m_revID.empty() ? "NoRef" : "Same"},
                        {"@@REV_ID@@", m_revID},
                        {"@@ON_ACCESS_STATUS@@", m_onAccessEnabled.load() ? "true" : "false"},
                        {"@@PLUGIN_VERSION@@", common::getPluginVersion()}
                });

        LOGDEBUG("Generated status XML for revId:" << m_revID);

        return result;
    }

    bool PluginCallback::sendStatus(const std::string &revID)
    {
        if (revID != m_revID)
        {
            LOGDEBUG("Received new policy with revision ID: " << revID);
            m_revID = revID;
            return sendStatus();
        }
        return false;
    }

    bool PluginCallback::sendStatus()
    {
        auto newStatus = generateSAVStatusXML();
        if (newStatus != m_savStatus)
        {
            m_savStatus = newStatus;
            m_task->push(Task{ .taskType=Task::TaskType::SendStatus, .Content=newStatus });
            return true;
        }
        return false;
    }

    void PluginCallback::setRunning(bool running)
    {
        m_running.store(running);
    }

    bool PluginCallback::isRunning()
    {
        return m_running.load();
    }

    std::string PluginCallback::extractValueFromProcStatus(const std::string& procStatusContent, const std::string& key)
    {
        auto contents = Common::UtilityImpl::StringUtils::splitString(procStatusContent, "\n");

        for (auto const& line : contents)
        {
            if (Common::UtilityImpl::StringUtils::startswith(line, key + ":"))
            {
                std::vector<std::string> list = Common::UtilityImpl::StringUtils::splitString(line, ":");
                std::string s = list[1];
                s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char c) { return !std::isspace(c); }));
                return s;
            }
        }
        return "";
    }

    long PluginCallback::calculateHealth(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls)
    {
        calculateThreatDetectorHealthStatus(sysCalls);
        calculateSoapHealthStatus(sysCalls);
        calculateSafeStoreHealthStatus(sysCalls);

        auto newHealth = m_safestoreServiceStatus == E_HEALTH_STATUS_GOOD &&
                         m_soapServiceStatus == E_HEALTH_STATUS_GOOD &&
                         m_threatDetectorServiceStatus == E_HEALTH_STATUS_GOOD ? E_HEALTH_STATUS_GOOD : E_HEALTH_STATUS_BAD;

        if (m_serviceHealth != newHealth)
        {
            std::string message = newHealth == E_HEALTH_STATUS_GOOD ? "green" : "red";
            LOGINFO("Service Health has changed to: " << message);

            m_serviceHealth = newHealth;
        }

        return newHealth;
    }

    void PluginCallback::calculateSafeStoreHealthStatus(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        if (!m_safeStoreEnabled)
        {
            m_safestoreServiceStatus = E_HEALTH_STATUS_GOOD;
        }
        else
        {
            bool dormant = fileSystem->isFile(getSafeStoreDormantFlagPath());
            if (!common::PidLockFile::isPidFileLocked(getSafeStorePidPath(), sysCalls) )
            {
                if (m_safestoreServiceStatus == E_HEALTH_STATUS_GOOD)
                {
                    LOGWARN("Sophos SafeStore Process is not running, turning service health to red");
                }
                m_safestoreServiceStatus = E_HEALTH_STATUS_BAD;
            }
            else if(dormant)
            {
                if (m_safestoreServiceStatus == E_HEALTH_STATUS_GOOD)
                {
                    LOGWARN("Sophos SafeStore Process failed initialisation, turning service health to red");
                }
                m_safestoreServiceStatus = E_HEALTH_STATUS_BAD;
            }
            else
            {
                if (m_safestoreServiceStatus == E_HEALTH_STATUS_BAD)
                {
                    LOGINFO("Sophos SafeStore Process is now healthy");
                }
                m_safestoreServiceStatus = E_HEALTH_STATUS_GOOD;
            }
        }
    }

    void PluginCallback::calculateSoapHealthStatus(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        bool unhealthy = fileSystem->isFile(getOnAccessUnhealthyFlagPath());
        if (unhealthy)
        {
            if(m_soapServiceStatus == E_HEALTH_STATUS_GOOD)
            {
                LOGWARN("Sophos On Access Process is unhealthy, turning service health to red");
            }
            m_soapServiceStatus = E_HEALTH_STATUS_BAD;
        }
        else if (!common::PidLockFile::isPidFileLocked(getSoapdPidPath(), sysCalls))
        {
            if(m_soapServiceStatus == E_HEALTH_STATUS_GOOD)
            {
                LOGWARN("Sophos On Access Process is not running, turning service health to red");
            }
            m_soapServiceStatus = E_HEALTH_STATUS_BAD;
        }
        else
        {
            if(m_soapServiceStatus == E_HEALTH_STATUS_BAD)
            {
                LOGINFO("Sophos On Access Process is now healthy");
            }
            m_soapServiceStatus = E_HEALTH_STATUS_GOOD;
        }
    }

    long PluginCallback::calculateThreatDetectorHealthStatus(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        Path threatDetectorPidFile = common::getPluginInstallPath() / "chroot/var/threat_detector.pid";
        bool unhealthy = fileSystem->isFile(getThreatDetectorUnhealthyFlagPath());
        if (unhealthy)
        {
            if(m_threatDetectorServiceStatus == E_HEALTH_STATUS_GOOD)
            {
                LOGWARN("Sophos Threat Detector Process is unhealthy, turning service health to red");
            }
            m_threatDetectorServiceStatus = E_HEALTH_STATUS_BAD;
        }
        else if (!common::PidLockFile::isPidFileLocked(threatDetectorPidFile, sysCalls) && !shutdownFileValid(fileSystem))
        {
            if(m_threatDetectorServiceStatus == E_HEALTH_STATUS_GOOD)
            {
                LOGWARN("Sophos Threat Detector Process is not running, turning service health to red");
            }
            m_threatDetectorServiceStatus = E_HEALTH_STATUS_BAD;
        }
        else if (susiUpdateFailed())
        {
            if(m_threatDetectorServiceStatus == E_HEALTH_STATUS_GOOD)
            {
                LOGWARN("SUSI update failed, turning service health to red");
            }
            m_threatDetectorServiceStatus = E_HEALTH_STATUS_BAD;
        }
        else
        {
            if(m_threatDetectorServiceStatus == E_HEALTH_STATUS_BAD)
            {
                LOGINFO("Sophos Threat Detector Process is now healthy");
            }

            m_threatDetectorServiceStatus = E_HEALTH_STATUS_GOOD;
        }
        return m_threatDetectorServiceStatus;
    }

    std::string PluginCallback::getHealth()
    {
        auto sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
        nlohmann::json j;
        j["Health"] = calculateHealth(sysCalls);
        return j.dump();
    }


    void PluginCallback::setSafeStoreEnabled(bool isEnabled)
    {
        m_safeStoreEnabled = isEnabled;
    }

    void PluginCallback::setOnAccessEnabled(bool isEnabled)
    {
        m_onAccessEnabled.store(isEnabled);
    }

} // namespace Plugin