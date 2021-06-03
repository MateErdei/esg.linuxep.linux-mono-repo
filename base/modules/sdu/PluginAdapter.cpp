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
        //m_callback->setRunning(true);
        LOGINFO("Entering the main loop");

        while (true)
        {
            Task task = m_queueTask->pop();
            switch (task.taskType)
            {
                case Task::TaskType::STOP:
                    LOGDEBUG("Process task STOP");
                    return;
                case Task::TaskType::ACTION:
                    LOGDEBUG("Process task ACTION");
                    processAction(task.Content);
                    break;
                case Task::TaskType::DiagnoseFailedToStart:
                    break;
                case Task::TaskType::DiagnoseMonitorDetached:
                    break;
                case Task::TaskType::DiagnoseFinished:
                    break;
                case Task::TaskType::DiagnoseTimedOut:
                    break;

            }
        }
        LOGSUPPORT("Left the plugin adapter main thread");
    }

    void PluginAdapter::processAction(const std::string& actionXml)
    {
        LOGINFO("processing action: " << actionXml);
        Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(actionXml);

        auto action = attributesMap.lookup("a:action");
        std::string actionType(action.value("type") );
        if (actionType != "SDURun")
        {
            std::stringstream errorMessage;
            errorMessage << "Malformed action received , type is : " << actionType <<" not SDURun";
            throw std::runtime_error(errorMessage.str());
        }

//        std::string url = action.value("uploadUrl");
//        std::string jsonString = "{\"url\":\""+url+"\"}";
//        auto fileSystem = Common::FileSystem::fileSystem();
//
//        fileSystem->writeFileAtomically(Common::ApplicationConfiguration::applicationPathManager().getDiagnoseConfig(),jsonString,Common::ApplicationConfiguration::applicationPathManager().getTempPath());
        //start diagnose
        std::string versionFile = Common::ApplicationConfiguration::applicationPathManager().getVersionIniFileForComponent("ServerProtectionLinux-Base-component");
        std::string version = Common::UtilityImpl::StringUtils::extractValueFromIniFile(versionFile, "PRODUCT_VERSION");
        std::string newStatus = replaceAll(statusTemplate,"@VERSION@",version);
        newStatus = replaceAll(statusTemplate,"@IS_RUNNING@","1");
        m_baseService->sendStatus("SDU",newStatus,newStatus);
        if (!m_diagnoseRunner->isRunning())
        {
            m_diagnoseRunner->triggerDiagnose();
        }
        //wait for it finish
        // times out- kill sd
        newStatus = replaceAll(statusTemplate,"@IS_RUNNING@","0");
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
