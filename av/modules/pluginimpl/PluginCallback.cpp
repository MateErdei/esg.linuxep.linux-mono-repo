/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallback.h"

#include "Logger.h"
#include "common/StringUtils.h"
#include "common/PluginUtils.h"
#include "datatypes/sophos_filesystem.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/XmlUtilities/AttributesMap.h>

#include <thirdparty/nlohmann-json/json.hpp>
#include <fstream>
#include <unistd.h>

namespace fs = sophos_filesystem;

namespace Plugin
{
    PluginCallback::PluginCallback(std::shared_ptr<QueueTask> task)
    : m_task(std::move(task))
    , m_revID(std::string())
    {
        std::string noPolicySetStatus = generateSAVStatusXML();
        m_statusInfo = { noPolicySetStatus, noPolicySetStatus, "SAV" };
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string& policyXml)
    {
        LOGSUPPORT("Applying new policy");
        m_task->push(Task{ Task::TaskType::Policy, policyXml });
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
        int timeoutCounter = 0;
        int shutdownTimeout = 30;
        while(isRunning() && timeoutCounter < shutdownTimeout)
        {
            LOGSUPPORT("Shutdown waiting for all processes to complete");
            sleep(1);
            timeoutCounter++;
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
        telemetry.set("health", static_cast<long> (getServiceHealth()));

        return telemetry.serialiseAndReset();
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
        fs::path vvfFile(appConfig.getData("PLUGIN_INSTALL"));
        vvfFile /= "chroot/susi/update_source/vdl/vvf.xml";

        std::ifstream vvfFileFs (vvfFile, std::ifstream::in);
        std::string virusDataVersion = "unknown";
        if (vvfFileFs.good())
        {
            std::stringstream vvfFileContents;
            vvfFileContents << vvfFileFs.rdbuf();

            auto attributeMap = Common::XmlUtilities::parseXml(vvfFileContents.str());
            virusDataVersion = attributeMap.lookup("VVF/VirusData").value("Version");
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
            m_task->push(Task{.taskType=Task::TaskType::SendStatus, generateSAVStatusXML()});
        }
    }

    void PluginCallback::setRunning(bool running)
    {
        m_running = running;
    }

    bool PluginCallback::isRunning()
    {
        return m_running;
    }

    int PluginCallback::getServiceHealth()
    {
        return m_health;
    }

    void PluginCallback::setServiceHealth(int health)
    {
        m_health = health;
    }

    void PluginCallback::calculateHealth()
    {
        // Infer that AV plugin healthy if this code can be reached.
        setServiceHealth(0);
    }

    std::string PluginCallback::getHealth()
    {
        nlohmann::json j;
        calculateHealth();
        j["Health"] = getServiceHealth();
        return j.dump();
    }

} // namespace Plugin