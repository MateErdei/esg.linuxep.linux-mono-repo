// Copyright 2020-2022, Sophos Limited.  All rights reserved.

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
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <datatypes/SystemCallWrapper.h>
#include <thirdparty/nlohmann-json/json.hpp>
// Std C++
#include <cstdlib>
#include <fstream>
#include <thread>
#include <chrono>
// Std C
#include <unistd.h>

namespace fs = sophos_filesystem;

using namespace std::chrono_literals;

namespace Plugin
{
    PluginCallback::PluginCallback(std::shared_ptr<QueueTask> task)
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

        struct stat statbuf{};
        int ret = stat("/opt/test/inputs", &statbuf);
        bool testing = (ret == 0);

        auto deadline = std::chrono::steady_clock::now() + 30s;
        if (testing)
        {
            deadline = std::chrono::steady_clock::now() + 8s;
        }
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
        if (!isRunning())
        {
            // Successful exit
            return;
        }
        LOGFATAL("AV Plugin has hung: main thread has not exited");
        if (testing)
        {
            std::this_thread::sleep_for(50ms);  // To allow logging to complete
            // Will cause a SIGABRT, and the default action for that is generating a Core File
            abort();
        }
        else
        {
            // Force exit
            exit(30);
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
        telemetry.set("lr-data-hash", getLrDataHash());
        telemetry.set("ml-lib-hash", getMlLibHash());
        telemetry.set("ml-pe-model-version", getMlModelVersion());
        telemetry.set("vdl-ide-count", getIdeCount());
        telemetry.set("vdl-version", getVirusDataVersion());
        telemetry.set("version", common::getPluginVersion());
        telemetry.set("sxl4-lookup", m_lookupEnabled);
        auto sysCalls = std::make_shared<datatypes::SystemCallWrapper>();
        telemetry.set("health", calculateHealth(sysCalls));
        telemetry.set("threatHealth", m_threatStatus);

        auto processInfo = getThreatScannerProcessinfo(sysCalls);
        telemetry.set("threatMemoryUsage", processInfo.first);
        telemetry.set("threatProcessAge", processInfo.second);

        telemetry.increment("scan-now-count", 0ul);
        telemetry.increment("scheduled-scan-count", 0ul);
        telemetry.increment("threat-count", 0ul);
        telemetry.increment("threat-eicar-count", 0ul);

        std::string telemetryJson = telemetry.serialiseAndReset();
        telemetry.set("threatHealth", m_threatStatus);
        return telemetryJson;
    }

    std::string PluginCallback::getLrDataHash()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path filerep(appConfig.getData("PLUGIN_INSTALL"));
        filerep /= "chroot/susi/update_source/reputation/filerep.dat";
        fs::path signerrep(appConfig.getData("PLUGIN_INSTALL"));
        signerrep /= "chroot/susi/update_source/reputation/signerrep.dat";

        std::ifstream filerepFs(filerep, std::ifstream::in);
        std::ifstream signerrepFs(signerrep, std::ifstream::in);

        if (filerepFs.good() && signerrepFs.good())
        {
            std::stringstream lrDataContents;
            lrDataContents << filerepFs.rdbuf() << signerrepFs.rdbuf();
            return common::sha256_hash(lrDataContents.str());
        }
        return "unknown";
    }

    std::string PluginCallback::getMlLibHash()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path mlLib(appConfig.getData("PLUGIN_INSTALL"));
        mlLib /= "chroot/susi/update_source/mllib/libmodel.so";
        std::ifstream mlLibFs (mlLib, std::ifstream::in);

        if(mlLibFs.good())
        {
            std::stringstream mlLibContents;
            mlLibContents << mlLibFs.rdbuf();
            return common::sha256_hash(mlLibContents.str());
        }
        return "unknown";
    }

    unsigned long PluginCallback::getIdeCount()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path vdbDir(appConfig.getData("PLUGIN_INSTALL"));
        vdbDir /= "chroot/susi/update_source/vdl";

        unsigned long ideCount = 0;
        if (fs::is_directory(vdbDir))
        {
            auto constexpr ide_filter = [](const fs::path& p) {
              return p.extension() == ".ide";
            };

            try
            {
                ideCount = std::count_if(fs::directory_iterator(vdbDir), fs::directory_iterator{}, static_cast<bool(*)(const fs::path&)>(ide_filter) );
            }
            catch (const fs::filesystem_error& e)
            {
                LOGERROR("Failed to count the number of IDES: " << e.what());
            }
        }

        return ideCount;
    }


    std::string PluginCallback::getVirusDataVersion()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        std::string virusDataVersion = "unknown";
        fs::path datasetAmfst(appConfig.getData("PLUGIN_INSTALL"));
        datasetAmfst /= "chroot/susi/update_source/vdl/manifestdata.dat";
        if (fs::exists(datasetAmfst))
        {
            virusDataVersion = "DataSet-A";
        }
        return virusDataVersion;
    }

    std::string PluginCallback::getMlModelVersion()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path mlModel(appConfig.getData("PLUGIN_INSTALL"));
        mlModel /= "chroot/susi/update_source/model/model.dat";

        std::ifstream in( mlModel, std::ios::binary );
        std::string versionStr = "unknown";

        if (in.good())
        {
            // The version number is 4 bytes long starting from an offset of 28 and is little-endian
            std::uint32_t version;
            in.seekg(28*sizeof(char));
            in.read(reinterpret_cast<char*>(&version), sizeof(version));
            versionStr = std::to_string(version);
        }
        in.close();

        return versionStr;
    }

    bool PluginCallback::shutdownFileValid() const
    {
        // Threat detector is expected to shut down periodically but should be restarted by watchdog after 10 seconds

        auto fileSystem = Common::FileSystem::fileSystem();

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

    bool PluginCallback::safeStoreEnabled()
    {
        bool enabled = false;
        auto fileSystem = Common::FileSystem::fileSystem();

        auto ssFlagsJson = fileSystem->readFile(Plugin::getSafeStoreFlagPath());
        auto parsedSSFlag = nlohmann::json::parse(ssFlagsJson);

        if (parsedSSFlag.value("ss_enabled", false))
        {
           enabled = true;
        }
        return enabled;
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
  <on-access>false</on-access>
  <entity>
    <productId>SSPL-AV</productId>
    <product-version>@@PLUGIN_VERSION@@</product-version>
    <entityInfo>SSPL-AV</entityInfo>
  </entity>
</status>)sophos",{
                        {"@@POLICY_COMPLIANCE@@", m_revID.empty() ? "NoRef" : "Same"},
                        {"@@REV_ID@@", m_revID},
                        {"@@PLUGIN_VERSION@@", common::getPluginVersion()}
                });

        LOGDEBUG("Generated status XML for revId:" << m_revID);

        return result;
    }

    void PluginCallback::sendStatus(const std::string &revID)
    {
        if (revID != m_revID)
        {
            LOGDEBUG("Received new policy with revision ID: " << revID);
            m_revID = revID;
            m_task->push(Task{ .taskType=Task::TaskType::SendStatus, .Content=generateSAVStatusXML() });
        }
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

    long PluginCallback::calculateHealth(const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls)
    {
        Path threatDetectorPidFile = common::getPluginInstallPath() / "chroot/var/threat_detector.pid";
        if (!common::PidLockFile::isPidFileLocked(threatDetectorPidFile, sysCalls) && !shutdownFileValid())
        {
            return E_HEALTH_STATUS_BAD;
        }

        Path soapdPidFile = common::getPluginInstallPath() / "var/soapd.pid";
        if (!common::PidLockFile::isPidFileLocked(soapdPidFile, sysCalls))
        {
            return E_HEALTH_STATUS_BAD;
        }

        Path safeStorePidFile = common::getPluginInstallPath() / "var/safestore.pid";
        if (!common::PidLockFile::isPidFileLocked(safeStorePidFile, sysCalls) && !safeStoreEnabled())
        {
            return E_HEALTH_STATUS_BAD;
        }

        return E_HEALTH_STATUS_GOOD;
    }

    std::string PluginCallback::getHealth()
    {
        auto sysCalls = std::make_shared<datatypes::SystemCallWrapper>();
        nlohmann::json j;
        j["Health"] = calculateHealth(sysCalls);
        return j.dump();
    }

    std::pair<unsigned long , unsigned long> PluginCallback::getThreatScannerProcessinfo(const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls)
    {
        const int expectedStatFileSize = 52;
        const int rssEntryInStat = 24;
        const int startTimeEntryInStat = 22;

        unsigned long memoryUsage = 0;
        unsigned long runTime = 0;
        std::optional<std::string> statFileContents;

        auto fileSystem = Common::FileSystem::fileSystem();
        Path threatDetectorPidFile = common::getPluginInstallPath() / "chroot/var/threat_detector.pid";
        int pid = getProcessPidFromFile(fileSystem, threatDetectorPidFile);

        if (pid == 0)
        {
            return std::pair(0,0);
        }

        try
        {
            statFileContents = fileSystem->readProcFile(pid, "stat");
            auto procStat = Common::UtilityImpl::StringUtils::splitString(statFileContents.value(), { ' ' });
            if (procStat.size() != expectedStatFileSize)
            {
                LOGDEBUG("The proc stat for " << pid << " is not of expected size");
                LOGERROR("Failed to get Sophos Threat Detector Process Info");
                return std::pair(0, 0);
            }

            memoryUsage = Common::UtilityImpl::StringUtils::stringToLong(procStat.at(rssEntryInStat - 1)).first;

            auto sysUpTime = sysCalls->getSystemUpTime();
            if (sysUpTime.first == -1)
            {
                LOGWARN("Failed to retrieve system info, cant calculate process duration. Returning memory usage only.");
                return std::pair(memoryUsage, 0);
            }
            long startTime = Common::UtilityImpl::StringUtils::stringToLong(procStat.at(startTimeEntryInStat - 1)).first/sysconf(_SC_CLK_TCK);
            runTime = sysUpTime.second - startTime;

        }
        catch (const std::bad_optional_access& e)
        {
            LOGERROR("Stat file of Pid: " << pid << " is empty. Cannot report on Threat Detector Process to Telemetry: " << e.what());
            return std::pair(0,0);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Error reading threat detector stat proc file due to: " << e.what());
            return std::pair(0,0);
        }

        return { memoryUsage, runTime };
    }

    int PluginCallback::getProcessPidFromFile(Common::FileSystem::IFileSystem* fileSystem, const Path& pidFilePath)
    {
        int pid;
        std::string pidAsString;

        try
        {
            pidAsString = fileSystem->readFile(pidFilePath);

            std::pair<int, std::string> pidStringToIntResult =
                    Common::UtilityImpl::StringUtils::stringToInt(pidAsString);
            if (!pidStringToIntResult.second.empty())
            {
                LOGWARN("Failed to read Pid file to int due to: " << pidStringToIntResult.second);
                return 0;
            }
            pid = pidStringToIntResult.first;
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGWARN("Error accessing pid file: " << pidFilePath << " due to: " << e.what());
            return 0;
        }

        try
        {
            if (!fileSystem->isDirectory("/proc/" + pidAsString))
            {
                LOGDEBUG("Process no longer running: " << pidAsString);
                return 0;
            }
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGWARN("Error accessing proc directory of pid: " << pidAsString << " due to: " << e.what());
            return 0;
        }

        return pid;
    }

} // namespace Plugin