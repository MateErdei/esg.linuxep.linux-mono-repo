/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"
#include "Logger.h"


#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/HttpSender/RequestConfig.h>
#include <CommsComponent/HttpRequester.h>

namespace
{

    std::string statusTemplate { R"sophos(<?xml version="1.0" encoding="utf-8" ?>
    <status version="@VERSION@" is_running="@IS_RUNNING@" />)sophos" };


    std::string replaceAll(const std::string& pattern, const std::string& key, const std::string& replace)
    {
        if (key.empty())
        {
            return pattern;
        }
        std::string result;
        size_t beginPos = 0;

        while (true)
        {
            size_t pos = pattern.find(key, beginPos);

            if (pos == std::string::npos)
            {
                break;
            }
            result += pattern.substr(beginPos, pos - beginPos);
            result += replace;
            beginPos = pos + key.length();
        }

        result += pattern.substr(beginPos);

        return result;
    }

} // namespace

namespace RemoteDiagnoseImpl
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<ITaskQueue> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback,
        std::unique_ptr<IAsyncDiagnoseRunner> diagnoseRunner) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_diagnoseRunner(std::move(diagnoseRunner))
    {
    }

    void PluginAdapter::mainLoop()
    {
        LOGINFO("Entering the main loop");

        while (true)
        {
            Task task = m_queueTask->pop(m_processing);
            switch (task.taskType)
            {
                case Task::TaskType::STOP:
                    LOGDEBUG("Process task STOP");
                    m_processing = false;
                    return;
                case Task::TaskType::ACTION:
                    LOGDEBUG("Process task ACTION");
                    m_processing = true;
                    processAction(task.Content);
                    break;
                case Task::TaskType::DIAGNOSE_FAILED_TO_START:
                    LOGDEBUG("Process task DiagnoseFailedToStart");
                    m_processing = false;
                    break;
                case Task::TaskType::DIAGNOSE_MONITOR_DETACHED:
                    LOGDEBUG("Process task DiagnoseMonitorDetached");
                    m_processing = false;
                    break;
                case Task::TaskType::DIAGNOSE_FINISHED:
                    LOGDEBUG("Process task DiagnoseFinished");
                    processZip();
                    m_processing = false;
                    break;
                case Task::TaskType::DIAGNOSE_TIMED_OUT:
                    LOGDEBUG("Process task DiagnoseTimedOut");
                    sendFinishedStatus();
                    m_processing = false;
                    break;
                case Task::TaskType::UNDEFINED:
                    break;
                default:
                    break;

            }
        }
        LOGSUPPORT("Left the plugin adapter main thread");
    }

    void PluginAdapter::processAction(const std::string& actionXml) {
        LOGDEBUG("Processing action: " << actionXml);
        Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(actionXml);

        auto action = attributesMap.lookup("a:action");
        std::string actionType(action.value("type"));
        if (actionType != "SDURun") {
            std::stringstream errorMessage;
            errorMessage << "Malformed action received , type is : " << actionType << " not SDURun";
            throw std::runtime_error(errorMessage.str());
        }

        m_url = action.value("uploadUrl");
        LOGDEBUG("Upload url: " << m_url);

        std::string versionFile = Common::ApplicationConfiguration::applicationPathManager().getVersionIniFileForComponent(
                "ServerProtectionLinux-Base-component");
        std::string version = Common::UtilityImpl::StringUtils::extractValueFromIniFile(versionFile, "PRODUCT_VERSION");

        std::string newStatus = replaceAll(statusTemplate, "@VERSION@", version);
        newStatus = replaceAll(newStatus, "@IS_RUNNING@", "1");
        //m_baseService->sendStatus("SDU", newStatus, newStatus);

        if (!m_diagnoseRunner->isRunning())
        {
            m_diagnoseRunner->triggerDiagnose();
        }

    }

    void PluginAdapter::processZip()
    {
        LOGINFO("Diagnose finished");
        std::string output = Common::ApplicationConfiguration::applicationPathManager().getDiagnoseOutputPath();
        auto fs = Common::FileSystem::fileSystem();
        std::string filepath = Common::FileSystem::join(output,"sspl.zip");

        UrlData data = processUrl();
        std::string processedfilepath = Common::FileSystem::join(output,data.filename);

        try
        {
            fs->moveFile(filepath, processedfilepath);
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("failed to process zip file due to error: " << exception.what());
        }
        std::string chrootPath = Common::FileSystem::join("/base/remote-diagnose/output",data.filename);
        Common::HttpSender::RequestConfig requestConfig{Common::HttpSender::RequestType::PUT,
                                                        std::vector<std::string>{},data.domain,
                                                        443,"",data.resourcePath,
                                                        chrootPath};

        try
        {
            auto response = CommsComponent::HttpRequester::triggerRequest("UploadDiagnose", requestConfig, std::chrono::minutes(1));

            if (response.httpCode != 200)
            {
                LOGINFO("Response HttpCode: " << response.httpCode);
                LOGINFO(response.description);

            }

        }
        catch (std::exception& ex)
        {
            LOGERROR("Perform request failed: " << ex.what());

        }
        sendFinishedStatus();
    }

    PluginAdapter::UrlData PluginAdapter::processUrl()
    {
        UrlData data;
        data.filename = Common::FileSystem::basename(m_url);
        if (!Common::UtilityImpl::StringUtils::startswith(m_url,"https://"))
        {
            throw std::runtime_error("Malformed url missing protocol");
        }
        std::string noProtocol = m_url.substr(8);
        data.resourcePath = noProtocol.substr(noProtocol.find_first_of("/"));
        data.domain = noProtocol.substr(0,noProtocol.find_first_of("/"));

        return data;
    }
    void PluginAdapter::sendFinishedStatus()
    {
        //TODO LINUXDAR-871 update mcs to handle more than one status sent during one command poll time
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::string versionFile = Common::ApplicationConfiguration::applicationPathManager().getVersionIniFileForComponent(
                "ServerProtectionLinux-Base-component");
        std::string version = Common::UtilityImpl::StringUtils::extractValueFromIniFile(versionFile, "PRODUCT_VERSION");

        std::string newStatus = replaceAll(statusTemplate, "@VERSION@", version);
        newStatus = replaceAll(newStatus,"@IS_RUNNING@","0");
        m_baseService->sendStatus("SDU",newStatus,newStatus);
    }

    PluginAdapter::~PluginAdapter()
    {
        try
        {
            if (m_monitor.valid())
            {
                // TODO stop the sophos_diagnose
            }
        }catch (std::exception& ex)
        {
            std::cerr << "Plugin adapter exception: " << ex.what() << std::endl;
        }
        // safe to clean up.
    }

} // namespace RemoteDiagnoseImpl
