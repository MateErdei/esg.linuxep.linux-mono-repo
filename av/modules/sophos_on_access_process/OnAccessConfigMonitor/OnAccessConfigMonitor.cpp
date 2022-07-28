/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OnAccessConfigMonitor.h"

#include "Logger.h"
#include "common/FDUtils.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

#include <thirdparty/nlohmann-json/json.hpp>

#include <utility>

using json = nlohmann::json;
using namespace sophos_on_access_process::ConfigMonitorThread;
namespace fs = sophos_filesystem;

OnAccessConfigMonitor::OnAccessConfigMonitor(std::string processControllerSocket) :
    m_processControllerSocketPath(std::move(processControllerSocket)),
    m_processControllerServer(m_processControllerSocketPath, 0666)
{
}

void OnAccessConfigMonitor::run()
{
    announceThreadStarted();

    m_processControllerServer.start();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;


    max = FDUtils::addFD(&readFDs, m_processControllerServer.monitorShutdownFd(), max);
    max = FDUtils::addFD(&readFDs, m_processControllerServer.monitorReloadFd(), max);

    while (true)
    {
        fd_set tempRead = readFDs;

        // wait for an activity on one of the fds
        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);
        if (activity < 0)
        {
            // error in pselect
            int error = errno;
            if (error == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from pselect");
                continue;
            }

            LOGERROR("Failed to read from socket - shutting down. Error: " << strerror(error) << " (" << error << ')');
            break;
        }

        if (FDUtils::fd_isset(m_processControllerServer.monitorShutdownFd(), &tempRead))
        {
            LOGINFO("Sophos On Access Process received shutdown request");
            m_processControllerServer.triggeredShutdown();
            break;
        }

        if (FDUtils::fd_isset(m_processControllerServer.monitorReloadFd(), &tempRead))
        {
            LOGINFO("Sophos On Access Process received configuration reload request");
            auto jsonString =  readConfigFile();
            parseOnAccessSettingsFromJson(jsonString);

            //update fanotify settings depending on parse result

            m_processControllerServer.triggeredReload();
        }
    }
}

std::string OnAccessConfigMonitor::readConfigFile()
{
    auto* sophosFsAPI =  Common::FileSystem::fileSystem();
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    auto configPath = pluginInstall / "var/soapd_config.json";

    try
    {
        std::string onAccessJsonConfig = sophosFsAPI->readFile(configPath.string());
        LOGDEBUG("New on-access configuration: " << onAccessJsonConfig);
        return  onAccessJsonConfig;
    }
    catch (const Common::FileSystem::IFileSystemException& ex)
    {
        LOGWARN("Failed to read on-access configuration, keeping existing configuration");
        return  "";
    }
}

OnAccessConfiguration OnAccessConfigMonitor::parseOnAccessSettingsFromJson(const std::string& jsonString)
{
    json parsedConfig;
    try
    {
        parsedConfig = json::parse(jsonString);

        OnAccessConfiguration configuration{};
        configuration.enabled = isSettingTrue(parsedConfig["enabled"]);
        configuration.excludeRemoteFiles = isSettingTrue(parsedConfig["excludeRemoteFiles"]);
        configuration.exclusions = parsedConfig["exclusions"].get<std::vector<std::string>>();

        LOGINFO("On-access enabled: " << parsedConfig["enabled"]);
        LOGINFO("On-access scan network: " << parsedConfig["excludeRemoteFiles"]);
        LOGINFO("On-access exclusions: " << parsedConfig["exclusions"]);

        return configuration;
    }
    catch (const json::parse_error& e)
    {
        LOGWARN("Failed to parse json configuration, reason: " << e.what());
    }

    return {};
}

bool OnAccessConfigMonitor::isSettingTrue(const std::string& settingString)
{
    if(settingString == "true")
    {
        return true;
    }

    //return false for any other possibility
    return false;
}
