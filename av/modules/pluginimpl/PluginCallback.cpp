/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallback.h"

#include "Logger.h"
#include "common/StringUtils.h"
#include "datatypes/sophos_filesystem.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/XmlUtilities/AttributesMap.h>

#include <fstream>

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
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string& appId)
    {
        LOGSUPPORT("Received get status request");
        assert(appId == "SAV");
        (void)appId;

        std::string status = generateSAVStatusXML();
        m_statusInfo = { status, status, "SAV" };
        return m_statusInfo;
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        telemetry.set("lr-data-hash", getLrDataHash());
        telemetry.set("ml-lib-hash", getMlLibHash());
        telemetry.set("vdl-ide-count", getIdeCount());
        telemetry.set("vdl-version", getVirusDataVersion());
        telemetry.set("version", getPluginVersion());

        return telemetry.serialiseAndReset();
    }

    std::string PluginCallback::getLrDataHash()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path filerep(appConfig.getData("PLUGIN_INSTALL"));
        filerep /= "chroot/susi/distribution_version/version1/lrdata/filerep.dat";
        fs::path signerrep(appConfig.getData("PLUGIN_INSTALL"));
        signerrep /= "chroot/susi/distribution_version/version1/lrdata/signerrep.dat";

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
        mlLib /= "chroot/susi/distribution_version/version1/libmodel.so";

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
        vdbDir /= "chroot/susi/distribution_version/version1/vdb";

        unsigned long ideCount = 0;
        if (fs::is_directory(vdbDir))
        {
            auto constexpr ide_filter = [](const fs::path& p) {
              return p.extension() == ".ide";
            };

            ideCount = std::count_if(fs::directory_iterator(vdbDir), fs::directory_iterator{}, static_cast<bool(*)(const fs::path&)>(ide_filter) );
        }

        return ideCount;
    }


    std::string PluginCallback::getVirusDataVersion()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path vvfFile(appConfig.getData("PLUGIN_INSTALL"));
        vvfFile /= "chroot/susi/distribution_version/version1/vdb/vvf.xml";

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

    std::string PluginCallback::getPluginVersion()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path versionFile(appConfig.getData("PLUGIN_INSTALL"));
        versionFile /= "VERSION.ini";

        std::string versionKeyword("PRODUCT_VERSION = ");
        std::ifstream versionFileFs(versionFile);
        if (versionFileFs.good())
        {
            std::string line;
            while (std::getline(versionFileFs, line))
            {
                if (line.rfind(versionKeyword, 0) == 0)
                {
                    return line.substr(versionKeyword.size(), line.size());
                }
            }
        }
        return "unknown";
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
                        {"@@PLUGIN_VERSION@@", getPluginVersion()}
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
} // namespace Plugin