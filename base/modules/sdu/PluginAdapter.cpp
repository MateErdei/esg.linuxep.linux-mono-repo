/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"
#include "Logger.h"


#include <Common/FileSystem/IFileSystem.h>
#include <Common/PluginApi/ApiException.h>

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
        Common::DirectoryWatcher::IiNotifyWrapperPtr iiNotifyWrapperPtr) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_iiNotifyWrapperPtr(std::move(iiNotifyWrapperPtr))

    {
    }

    void PluginAdapter::mainLoop()
    {
        //m_callback->setRunning(true);
        LOGINFO("Entering the main loop");
        // Request policy on startup

        // keep this to monitot for output file
//        MCSConfigDirListener mcsConfigDirListener(Common::ApplicationConfiguration::applicationPathManager().getMcsConfigFolderPath(), m_queueTask);
//        Common::DirectoryWatcher::IDirectoryWatcherPtr directoryWatcher = Common::DirectoryWatcher::createDirectoryWatcher(std::move(m_iiNotifyWrapperPtr));
//        directoryWatcher->addListener(mcsConfigDirListener);
//        directoryWatcher->startWatch();


        while (true)
        {
            Task task = m_queueTask->pop();
            switch (task.taskType)
            {
                case Task::TaskType::STOP:
                    LOGDEBUG("Process task STOP");
                    //directoryWatcher->removeListener(mcsConfigDirListener);
                    return;
                case Task::TaskType::ACTION:
                    LOGDEBUG("Process task ACTION");
                    processAction(task.Content);
                    break;

            }
        }
        LOGSUPPORT("Left the plugin adapter main thread");
    }

    void PluginAdapter::processAction(const std::string& actionXml)
    {
        LOGINFO("processing action: " << actionXml);
        // create arguments file
        //start diagnose
        //wait for it finish
        // times out- kill sd
        std::string newStatus = replaceAll(statusTemplate,"@IS_RUNNING@","0");
        m_callback->setStatus(Common::PluginApi::StatusInfo{newStatus,newStatus,"SDU"});
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
