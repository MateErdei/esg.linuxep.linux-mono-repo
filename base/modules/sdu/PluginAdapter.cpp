/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"
#include "PluginUtils.h"
#include "Logger.h"


#include <Common/FileSystem/IFileSystem.h>


#include <Common/PluginApi/ApiException.h>


#include <Common/FileSystem/IFileSystemException.h>


namespace
{






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
                    RemoteDiagnoseImpl::PluginUtils::processZip(m_url);
                    sendFinishedStatus();
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
        m_url = RemoteDiagnoseImpl::PluginUtils::processAction(actionXml);

        if (!m_diagnoseRunner->isRunning())
        {
            m_diagnoseRunner->triggerDiagnose();
        }

    }
    void PluginAdapter::sendFinishedStatus()
    {
        std::string status = RemoteDiagnoseImpl::PluginUtils::getFinishedStatus();
        m_baseService->sendStatus("SDU",status,status);
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
