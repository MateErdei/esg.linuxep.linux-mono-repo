// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "PluginAdapter.h"
#include "Logger.h"
#include "Telemetry/Telemetry.h"
#include "common/TelemetryConsts.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/XmlUtilities/AttributesMap.h"

// Bazel wants <json.hpp> but CMake wants <nlohmann/json.hpp>
#ifdef SPL_BAZEL
#include "AutoVersioningHeaders/AutoVersion.h"
#endif

#include <nlohmann/json.hpp>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_parallelSessionProcessor{ sessionrunner::createSessionRunner(Plugin::liveResponseExecutable()), Plugin::sessionsDirectoryPath() },
        m_mcsConfigPath(Common::ApplicationConfiguration::applicationPathManager().getMcsConfigFilePath())
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        #ifdef SPL_BAZEL
        LOGINFO("Live Response " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");
        #endif
        LOGINFO("Entering the main loop");
        Plugin::Telemetry::initialiseTelemetry();

        while (true)
        {
            Task task = m_queueTask->pop();
            switch (task.m_taskType)
            {
                case Task::TaskType::Stop:
                    return;

                case Task::TaskType::Policy:
                    LOGWARN("Live Response functionality is not affected by policy so unsolicited policy received");
                    break;

                case Task::TaskType::Response:
                    Common::Telemetry::TelemetryHelper::getInstance().increment(Plugin::Telemetry::totalSessionsTag, 1L);
                    std::string id = createJsonFile(task.m_content, task.m_correlationId);
                    if (id.empty())
                    {
                        LOGERROR("No id file present so unable to launch session");
                        Common::Telemetry::TelemetryHelper::getInstance().increment(Plugin::Telemetry::failedSessionsTag, 1L);
                        break;
                    }
                    startLiveResponseSession(task.m_correlationId);
                    break;
            }
        }
    }

    std::tuple<std::string, std::string> PluginAdapter::readXml(const std::string& xml)
    {
        using namespace Common::XmlUtilities;
        try
        {
            LOGDEBUG("Parsing xml (" << xml << ") for url and thumbprint");
            AttributesMap attributesMap = parseXml(xml);
            Attributes urlAttributes = attributesMap.lookup("action/url");
            Attributes thumbprintAttributes = attributesMap.lookup("action/thumbprint");
            std::string url = urlAttributes.contents();
            std::string thumbprint = thumbprintAttributes.contents();
            LOGDEBUG("Found url " << url << " and thumbprint " << thumbprint);
            return { url, thumbprint };
        }
        catch (XmlUtilitiesException& ex)
        {
            std::string reason = ex.what();
            LOGERROR("Failed to parse xml: " << reason);
        }
        return { "", "" };
    }


    void PluginAdapter::updateSessionConnectionDataTelemetry(const std::string& url, const std::string& thumbprint)
    {
        try
        {
            Common::Telemetry::TelemetryObject sessionObject;
            std::string hostname = Plugin::PluginUtils::getHostnameFromUrl(url);
            std::string key = hostname + ":" + thumbprint;
            std::string compoundKey = Plugin::Telemetry::sessionConnectionData + "." + key;
            Common::Telemetry::TelemetryHelper::getInstance().increment(compoundKey,1L);
            LOGDEBUG("Successfully incremented key: " + compoundKey);
        }
        catch (std::exception& ex)
        {
            LOGWARN("Failed to update" << Plugin::Telemetry::sessionConnectionData << " with error " << ex.what());
        }
    }

    std::string PluginAdapter::createJsonFile(const std::string& actionXml, const std::string& correlationId)
    {
        LOGDEBUG("Converting action xml into input json:" << actionXml);
        // Generate an ID for the filename and make sure it is not already present in
        // /opt/sophos-spl/plugins/liveresponse/var/ return id as a std::string
        auto fileSystem = Common::FileSystem::fileSystem();
        nlohmann::json jsonArray;

        auto [url, thumbprint] = readXml(actionXml);

        if (url.empty() || thumbprint.empty())
        {
            LOGWARN("Failed to find url and/or thumbprint in actionXml: " << actionXml);
            return {};
        }

        std::string filepath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().getBaseSophossplConfigFileDirectory()
            , "current_proxy");

        auto [proxy,credentials] = Plugin::PluginUtils::readCurrentProxyInfo(filepath, fileSystem);

        if (!proxy.empty())
        {
            LOGDEBUG("Writing proxy info to file");
            jsonArray["proxy"] = proxy;
            if (!credentials.empty())
            {
                LOGDEBUG("Writing credential info to file");
                jsonArray["credentials"] = credentials;
            }
        }

        jsonArray["url"] = url;
        jsonArray["thumbprint"] = thumbprint;

        std::optional<std::string> jwtToken = Plugin::PluginUtils::readKeyValueFromConfig(
            "jwt_token", m_mcsConfigPath);
        std::optional<std::string> deviceID = Plugin::PluginUtils::readKeyValueFromConfig(
            "device_id", m_mcsConfigPath);
        std::optional<std::string> tenantID = Plugin::PluginUtils::readKeyValueFromConfig(
            "tenant_id", m_mcsConfigPath);

        std::vector<std::string> missingMandatoryValuesList;
        if (!jwtToken.has_value()) {missingMandatoryValuesList.emplace_back("jwt_token");}
        if (!deviceID.has_value()) {missingMandatoryValuesList.emplace_back("device_id");}
        if (!tenantID.has_value()) {missingMandatoryValuesList.emplace_back("tenant_id");}

        if (!missingMandatoryValuesList.empty())
        {
            std::string missingMandatoryValues;
            for (auto &value : missingMandatoryValuesList)
            {
                missingMandatoryValues += value + ", " ;
            }
            // remove the ", " from the string
            missingMandatoryValues = missingMandatoryValues.substr(0,missingMandatoryValues.size()-2);
            LOGWARN("mcs.config is missing the following values: " + missingMandatoryValues);
            return {};
        }

        jsonArray["jwt_token"] = jwtToken.value();
        jsonArray["device_id"] = deviceID.value();
        jsonArray["tenant_id"] = tenantID.value();

        auto liveTerminalVersion = Common::UtilityImpl::FileUtils::extractValueFromFile(Plugin::getVersionIniFilePath(), "PRODUCT_VERSION");
        auto versionOS = Plugin::PluginUtils::getVersionOS();

        if (!liveTerminalVersion.first.empty())
            jsonArray["user_agent"] = "live-term/" + liveTerminalVersion.first + " (" + versionOS + ")";
        else
        {
            jsonArray["user_agent"] = "live-term/BAD-VERSION (" + versionOS + ")";
            LOGERROR("Could not read VERSION.ini of Liveterminal: " << liveTerminalVersion.second);
        }

        std::string jsonString;
        try
        {
             jsonString = jsonArray.dump();
        }
        catch(const std::exception& exception)
        {
            LOGERROR("Could not serialise session JSON object to string from XML:" << actionXml << ", with exception: " << exception.what());
            return {};
        }
        updateSessionConnectionDataTelemetry(url,thumbprint);
        std::string finalFilePath = Common::FileSystem::join(Plugin::sessionsDirectoryPath(), correlationId);

        LOGDEBUG("Creating json file " << finalFilePath << " with content: " << jsonString);

        fileSystem->writeFileAtomically(finalFilePath, jsonString, Plugin::liveResponseTempPath());

        if (!fileSystem->isFile(finalFilePath))
        {
            LOGWARN("File not created");
            return {};
        }
        return correlationId;
    }

    void PluginAdapter::startLiveResponseSession(const std::string& id) { m_parallelSessionProcessor.addJob(id); }
} // namespace Plugin