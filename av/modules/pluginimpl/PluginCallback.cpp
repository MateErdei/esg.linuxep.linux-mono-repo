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
        telemetry.set("ml-pe-model-hash", getMlModelHash());
        telemetry.set("ml-pe-model-version", getMlModelVersion());
        telemetry.set("version", getPluginVersion());

        return telemetry.serialiseAndReset();
    }

    std::string PluginCallback::getMlModelVersion()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path mlModel(appConfig.getData("PLUGIN_INSTALL"));
        mlModel /= "chroot/susi/distribution_version/version1/libmodel.so";

        std::ifstream in( mlModel, std::ios::binary );
        std::stringstream ss;

        if (in.good())
        {
            // The version number is 4 bytes long starting from an offset of 28 and is little-endian so we read it backwards
            int readOffsetBytes = 28;
            int readLength = 4;
            char c;
            for (int i = readOffsetBytes + readLength - 1; i >= readOffsetBytes; i--)
            {
                in.seekg(i*sizeof(char));
                in.get(c);
                ss << std::hex << static_cast<unsigned int>(c);
            }
        }
        in.close();

        unsigned int x;
        ss >> x;

        return std::to_string(x);
    }

    std::string PluginCallback::getMlModelHash()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path mlModel(appConfig.getData("PLUGIN_INSTALL"));
        mlModel /= "chroot/susi/distribution_version/version1/mlmodel/model.dat";

        std::ifstream ifs (mlModel, std::ifstream::in);
        std::string mlModelContents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

        return common::sha256_hash(mlModelContents);
    }

    std::string PluginCallback::getPluginVersion()
    {
        std::string pluginVersion = "Not Found";
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path versionFile(appConfig.getData("PLUGIN_INSTALL"));
        versionFile /= "VERSION.ini";

        if (fs::exists(versionFile))
        {
            std::string versionKeyword("PRODUCT_VERSION = ");
            std::ifstream versionFileHandle(versionFile);
            std::string line;
            while (std::getline(versionFileHandle, line))
            {

                if (line.rfind(versionKeyword, 0) == 0)
                {
                    pluginVersion = line.substr(versionKeyword.size(), line.size());
                }
            }
        }
        return pluginVersion;
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