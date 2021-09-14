/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"
#include "PluginUtils.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/PluginApi/ApiException.h>

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
                    sendStartedStatus();
                    LOGDEBUG("Waiting for running status to be sent.");
                    // We wait 120 seconds for the hope that the running status is sent.
                    // This is simply because we need to send 2 statuses one to say diagnose is running
                    // and another to state we are finished.  Diagnose can only take a few seconds which is too quick
                    // with regards to sending status message that will not overwrite each other and for
                    // the status messages to be processed by central.
                    std::this_thread::sleep_for(std::chrono::seconds (120));
                    LOGDEBUG("Starting diagnose");
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
        else
        {
            LOGWARN("sophos_diagnose is currently running, we will not try to start it again");
        }

    }
    void PluginAdapter::sendStartedStatus()
    {
        std::string status = RemoteDiagnoseImpl::PluginUtils::getStatus(1);
        m_baseService->sendStatus("SDU",status,status);
    }
    void PluginAdapter::sendFinishedStatus()
    {
        std::string status = RemoteDiagnoseImpl::PluginUtils::getStatus(0);
        m_baseService->sendStatus("SDU",status,status);
    }

    PluginAdapter::~PluginAdapter()
    {
        LOGDEBUG("SDU finished");
    }

} // namespace RemoteDiagnoseImpl
